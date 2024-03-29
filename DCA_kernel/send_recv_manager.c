// @file send_recv_manager.c
// @brief The functions to perform send and recv calls using customization modules

#include <linux/sched.h>
#include <linux/timekeeping.h> // for timestamps

#include "customization_socket.h"
#include "send_recv_manager.h"
#include "util/helpers.h"


int dca_sendmsg(struct customization_socket *cust_sock, struct sock *sk, struct msghdr *msg, size_t size,
                int (*sendmsg)(struct sock *, struct msghdr *, size_t))
{
    struct msghdr kmsg = *msg;
    struct kvec iov;
    struct customization_node *cust_node;
    struct iov_iter iter_temp = msg->msg_iter;
    bool cust_performed = false;
    int sendmsg_return;
    int i;


#ifdef DEBUG1
    trace_printk("L4.5: Start of DCA sendmsg, given size = %lu\n", size);
    trace_print_msg_params(msg);
#endif

    spin_lock(&cust_sock->active_customization_lock);

    cust_sock->send_buf_st.src_iter = &msg->msg_iter;
    cust_sock->send_buf_st.copy_length = 0; // default value
    cust_sock->send_buf_st.length = size;

    //TODO: should we sort modules here or prior to dca_sendmsg call?
    // Probably here b/c we hold the active customization lock

    // check if modules should be sorted before using them
    if (cust_sock->update_cust_sort)
    {
        sort_attached_cust(cust_sock);
        cust_sock->update_cust_sort = false;
    }

    // loop through cust modules until one performs a customization
    for (i = 0; i < MAX_CUST_ATTACH; i++)
    {
        cust_node = cust_sock->customizations[i];

        // customization could have been unregistered right before this call
        // or we reached the end of valid customization modules in the array
        if (cust_node == NULL)
        {
            if (i == 0)
            {
                // if i == 0, then we should not have found a NULL node
#ifdef DEBUG
                trace_printk("L4.5 ALERT: Cust_node NULL found unexpectedly, pid %d\n", cust_sock->pid);
#endif
                goto skip_and_send;
            }
            else
            {
                // reached a NULL node, so no cust is happening this round
                spin_unlock(&cust_sock->active_customization_lock);
                // trace_printk("released lock\n");
                return sendmsg(sk, msg, size);
            }
        }

        if (cust_sock->customize_or_skip == SKIP)
        {
#ifdef DEBUG
            trace_printk("L4.5 ALERT: Cust_node send SKIP set, pid %d\n", cust_sock->pid);
#endif
            goto skip_and_send;
        }

        // this case should never happen, but checking to ensure no future errors
        if (cust_node->send_function == NULL || cust_sock->send_buf_st.buf == NULL)
        {
#ifdef DEBUG
            trace_printk("L4.5 ALERT: send function or buffer set to NULL unexpectedly, pid %d\n", cust_sock->pid);
#endif
            goto skip_and_send;
        }

        // passes msg iter to customization send function
        cust_node->send_function(&cust_sock->send_buf_st, &cust_sock->socket_flow);

        // restore msg_iter values after cust module likely changed them
        msg->msg_iter = iter_temp;


#ifdef DEBUG2
        // this is for troubleshooting; iter should be at state given to send
        trace_printk("L4.5: After cust send module return\n");
        trace_print_iov_params(&msg->msg_iter);
#endif

        // handle case when cust mod wants to trigger send failure
        if (cust_sock->send_buf_st.copy_length < 0)
        {
#ifdef DEBUG1
            trace_printk("L4.5: send %lu error message from module, pid %d\n", cust_sock->send_buf_st.copy_length,
                         cust_sock->pid);
#endif

            spin_unlock(&cust_sock->active_customization_lock);
            return cust_sock->send_buf_st.copy_length;
        }

        // check if module will no longer be customizing the socket
        if (cust_sock->send_buf_st.set_cust_to_skip == true)
        {
#ifdef DEBUG
            trace_printk("L4.5: cust module send skip set, pid %d\n", cust_sock->pid);
#endif
            goto skip_and_send;
        }

        if (cust_sock->send_buf_st.try_next)
        {
            // go to the next customization node b/c current one couldn't match customization
            continue;
        }
        else
        {
            cust_performed = true;
            break;
        }
    }

    if (cust_performed == false)
    {
        spin_unlock(&cust_sock->active_customization_lock);
        return sendmsg(sk, msg, size);
    }

    // if no cust set, then we don't need to copy the buffer
    if (cust_sock->send_buf_st.no_cust == true)
    {
#ifdef DEBUG1
        trace_printk("L4.5: cust module no cust performed for send, pid %d\n", cust_sock->pid);
#endif

        spin_unlock(&cust_sock->active_customization_lock);
        return sendmsg(sk, msg, size);
    }



    // if copy length = 0, then we basically refuse to send the data but still lie that we did?
    // this allows the send side to effectively buffer data
    if (cust_sock->send_buf_st.copy_length == 0)
    {
#ifdef DEBUG
        trace_printk("L4.5: send cust module returned 0 bytes, pid %d\n", cust_sock->pid);
#endif

        spin_unlock(&cust_sock->active_customization_lock);
        // make buffer reflect that we actually sent everything we claimed to send
        iov_iter_advance(&msg->msg_iter, size);
        return size;
    }

    // TODO: Is this the best way to handle this case?
    // make sure send can succeed
    if (cust_sock->send_buf_st.copy_length > cust_sock->send_buf_st.buf_size)
    {
#ifdef DEBUG
        trace_printk("L4.5 ALERT Copy length problem: copy_length=%lu, buf_size=%u\n",
                     cust_sock->send_buf_st.copy_length, cust_sock->send_buf_st.buf_size);
#endif
#ifdef DEBUG2
        // this is for troubleshooting
        trace_printk("L4.5 ALERT: Copy error, just before return where I think I am giving original buffer to L4\n");
        trace_print_iov_params(&msg->msg_iter);
#endif

        // pretend that we never did anything to the message
        spin_unlock(&cust_sock->active_customization_lock);

        return sendmsg(sk, msg, size);
    }

#ifdef DEBUG2
    trace_printk("L4.5: Module adjusted send size = %lu\n", cust_sock->send_buf_st.copy_length);
#endif

    // create new iter based on cust send buffer to give to L4
    iov.iov_base = cust_sock->send_buf_st.buf;
    iov.iov_len = cust_sock->send_buf_st.copy_length;
    // this needs to be kvec since we are in kernel space; 1 = number of segments
    iov_iter_kvec(&kmsg.msg_iter, READ, &iov, 1, cust_sock->send_buf_st.copy_length);

    sendmsg_return = sendmsg(sk, &kmsg, cust_sock->send_buf_st.copy_length);
    spin_unlock(&cust_sock->active_customization_lock);

    if (sendmsg_return < 0)
    {
#ifdef DEBUG
        trace_printk("L4.5 ALERT: Sendmsg returned error code = %d, copy length = %lu\n", sendmsg_return,
                     cust_sock->send_buf_st.copy_length);
#endif
        return sendmsg_return;
    }
    else
    {
        // only update time after we send successfully
        ktime_get_real_ts64(&cust_sock->last_cust_send_time_struct);
#ifdef DEBUG2
        trace_printk("L4.5: Last send time %llu\n", cust_sock->last_cust_send_time_struct.tv_sec);
#endif
        // make buffer reflect that we actually sent everything we claimed to send
        iov_iter_advance(&msg->msg_iter, size);

        return size;
    }

skip_and_send:
#ifdef DEBUG
    trace_printk("L4.5: skip and send triggered\n");
#endif
    cust_sock->customize_or_skip = SKIP;
    spin_unlock(&cust_sock->active_customization_lock);
    // trace_printk("released lock\n");
    return sendmsg(sk, msg, size);
}
 


//TODO: is there a way to better modularize these functions?

int dca_recvmsg(struct customization_socket *cust_sock, struct sock *sk, struct msghdr *msg, size_t len,
                size_t recvmsg_ret)
{
    struct customization_node *cust_node;
    struct iov_iter iter_temp;
    size_t copy_success;
    bool cust_performed = false;
    int i;

#ifdef DEBUG1
    trace_printk("L4.5 Start of DCA recvmsg\n");
    trace_print_msg_params(msg);
#endif

    // this resets the msg iter buffer back to point at positions before L4 recvmsg
    // put data into the buffer; NOTE: recvmsg_ret is > 0
    iov_iter_revert(&msg->msg_iter, recvmsg_ret);
    iter_temp = msg->msg_iter;

#ifdef DEBUG2
    trace_printk("L4.5: Iter after reverting %lu bytes\n", recvmsg_ret);
    trace_print_iov_params(&msg->msg_iter);
#endif

    cust_sock->recv_buf_st.src_iter = &msg->msg_iter;
    cust_sock->recv_buf_st.copy_length = 0; // just to ensure it is defined
    cust_sock->recv_buf_st.length = len;
    cust_sock->recv_buf_st.recv_return = recvmsg_ret;

    // grab this lock to prevent a cust module from unregistering and creating
    // a NULL pointer problem
    spin_lock(&cust_sock->active_customization_lock);

    // check if modules should be sorted before using them
    if (cust_sock->update_cust_sort)
    {
        sort_attached_cust(cust_sock);
        cust_sock->update_cust_sort = false;
    }

    // loop through cust modules until one performs a customization
    for (i = 0; i < MAX_CUST_ATTACH; i++)
    {
        cust_node = cust_sock->customizations[i];

        if (cust_node == NULL)
        {
            if (i == 0)
            {
                // if i == 0, then we should not have found a NULL node
#ifdef DEBUG
                trace_printk("L4.5 ALERT: Cust_node NULL found unexpectedly, pid %d\n", cust_sock->pid);
#endif
                goto skip_and_recv;
            }
            else
            {
#ifdef DEBUG1
                trace_printk("L4.5: Reached a NULL Cust_node, pid %d\n", cust_sock->pid);
#endif
                // reached a NULL node, so no cust is happening this round
                goto advance_and_recv;
            }
        }

        if (cust_sock->customize_or_skip == SKIP)
        {
#ifdef DEBUG
            trace_printk("L4.5 ALERT: Cust_node recv SKIP set unexpectedly, pid %d\n", cust_sock->pid);
#endif
            goto skip_and_recv;
        }

        // this case should never happen, but possible if cust removed just before this call
        if (cust_node->recv_function == NULL || cust_sock->recv_buf_st.buf == NULL)
        {
#ifdef DEBUG
            trace_printk("L4.5 ALERT: recv function or buffer set to NULL unexpectedly, pid %d\n", cust_sock->pid);
#endif
            goto skip_and_recv;
        }


        // passes required data to cust_recv function
        cust_node->recv_function(&cust_sock->recv_buf_st, &cust_sock->socket_flow);

        // restore msg_iter values after cust module likely changed them
        msg->msg_iter = iter_temp;


#ifdef DEBUG2
        trace_printk("L4.5: After cust module return\n");
        trace_print_iov_params(&msg->msg_iter);
#endif


        // handle case when cust mod wants to trigger recv failure
        if (cust_sock->recv_buf_st.recv_return < 0)
        {
#ifdef DEBUG1
            trace_printk("L4.5: received %lu error message from module, pid %d\n", cust_sock->recv_buf_st.recv_return,
                         cust_sock->pid);
#endif
            spin_unlock(&cust_sock->active_customization_lock);
            return cust_sock->recv_buf_st.recv_return;
        }

        // check if module will no longer be customizing the socket
        if (cust_sock->recv_buf_st.set_cust_to_skip == true)
        {
#ifdef DEBUG
            trace_printk("L4.5: recv cust module skip set, pid %d\n", cust_sock->pid);
#endif
            goto skip_and_recv;
        }

        if (cust_sock->recv_buf_st.try_next)
        {
            // go to the next customization node b/c current one couldn't match customization
            continue;
        }
        else
        {
            cust_performed = true;
            break;
        }
    }

    if (cust_performed == false)
    {
#ifdef DEBUG
        trace_printk("L4.5: No cust performed, pid %d\n", cust_sock->pid);
#endif
        goto advance_and_recv;
    }

    // if cust didn't do anything, then no need to perform buffer operations
    if (cust_sock->recv_buf_st.no_cust == true)
    {
#ifdef DEBUG1
        trace_printk("L4.5: recv cust module no cust set, pid %d\n", cust_sock->pid);
#endif
        goto advance_and_recv;
    }



    if (cust_sock->recv_buf_st.copy_length == 0)
    {
#ifdef DEBUG
        trace_printk("L4.5: recv cust module returned 0 bytes, pid %d\n", cust_sock->pid);
#endif

        spin_unlock(&cust_sock->active_customization_lock);
        return -EAGAIN;
    }


    // make sure copy can succeed
    if (cust_sock->recv_buf_st.copy_length > cust_sock->recv_buf_st.buf_size ||
        cust_sock->recv_buf_st.copy_length > len)
    {
#ifdef DEBUG
        trace_printk("L4.5 ALERT Copy length problem: copy_length=%lu, buf_size=%u, app_len=%lu\n",
                     cust_sock->recv_buf_st.copy_length, cust_sock->recv_buf_st.buf_size, len);
#endif
        // pretend that we never did anything to the message
        goto advance_and_recv;
    }


#ifdef DEBUG2
    trace_printk("L4.5 just before copy_to_iter, desired bytes = %lu\n", cust_sock->recv_buf_st.copy_length);
    trace_print_iov_params(&msg->msg_iter);
#endif

    copy_success = copy_to_iter(cust_sock->recv_buf_st.buf, cust_sock->recv_buf_st.copy_length, &msg->msg_iter);

    // I don't think this is likely, but need to check
    if (copy_success != cust_sock->recv_buf_st.copy_length)
    {
#ifdef DEBUG
        trace_printk("L4.5 ALERT Copied bytes not same as desired value, copied = %lu, desired = %lu\n", copy_success,
                     cust_sock->recv_buf_st.copy_length);
    #ifdef DEBUG2
        trace_print_iov_params(&msg->msg_iter);
    #endif
#endif
        // TODO: somehow inform module that data not delivered to app
        //  reset iter?
        spin_unlock(&cust_sock->active_customization_lock);
        return -EAGAIN;
    }

#ifdef DEBUG2
    trace_printk("L4.5: After copy success, before return to app\n");
    trace_print_iov_params(&msg->msg_iter);
#endif

    // only update time after we recv successfully
    ktime_get_real_ts64(&cust_sock->last_cust_recv_time_struct);
#ifdef DEBUG2
    trace_printk("L4.5: Last recv time: %llu\n", cust_sock->last_cust_recv_time_struct.tv_sec);
#endif

    spin_unlock(&cust_sock->active_customization_lock);
    return cust_sock->recv_buf_st.copy_length;


skip_and_recv:
#ifdef DEBUG
    trace_printk("L4.5: skip and recv triggered\n");
    #ifdef DEBUG2
    trace_print_iov_params(&msg->msg_iter);
    #endif
#endif
    cust_sock->customize_or_skip = SKIP;
    iov_iter_advance(&msg->msg_iter, recvmsg_ret);
    spin_unlock(&cust_sock->active_customization_lock);
    return recvmsg_ret;

advance_and_recv:
#ifdef DEBUG1
    trace_printk("L4.5: advance and recv triggered\n");
    #ifdef DEBUG2
    trace_print_iov_params(&msg->msg_iter);
    #endif
#endif
    iov_iter_advance(&msg->msg_iter, recvmsg_ret);
    spin_unlock(&cust_sock->active_customization_lock);
    return recvmsg_ret;
}
