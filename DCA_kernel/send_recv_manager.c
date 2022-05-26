// @file send_recv_manager.c
// @brief The functions to perform send and recv calls using customization modules

#include <linux/sched.h>
#include <linux/timekeeping.h> // for timestamps

#include "send_recv_manager.h"
#include "util/printing.h"


int dca_sendmsg(struct customization_socket *cust_sock, struct sock *sk, struct msghdr *msg, size_t size,
                int (*sendmsg)(struct sock *, struct msghdr *, size_t))
{
    struct msghdr kmsg = *msg;
    struct kvec iov;
    struct customization_node *cust_node;
    struct iov_iter iter_temp = msg->msg_iter;
    int sendmsg_return;


#ifdef DEBUG1
    trace_printk("L4.5: Start of DCA sendmsg, given size = %lu\n", size);
    trace_print_msg_params(msg);
#endif

    spin_lock(&cust_sock->active_customization_lock);
    cust_node = cust_sock->customization;
    if (cust_node == NULL || cust_sock->customize_send_or_skip == SKIP)
    {
// customization could have been unregistered right before this call
#ifdef DEBUG
        trace_printk("L4.5 ALERT: Cust_node NULL or send SKIP set unexpectedly, pid %d\n", cust_sock->pid);
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


    cust_sock->send_buf_st.src_iter = &msg->msg_iter;
    cust_sock->send_buf_st.copy_length = 0; // default value
    cust_sock->send_buf_st.length = size;

    // passes msg iter to customization send function
    cust_node->send_function(&cust_sock->send_buf_st, &cust_sock->socket_flow);

    // restore msg_iter values after cust module likely changed them
    msg->msg_iter = iter_temp;

#ifdef DEBUG2
    // this is for troubleshooting; iter should be at state given to send
    trace_printk("L4.5: After cust send module return\n");
    trace_print_iov_params(&msg->msg_iter);
#endif

    // TODO: handle case when cust mod wants to trigger send failure?

    // cust module must have valid buffer pointer
    // modules could set to NULL to signal no longer customizing the socket
    if (cust_sock->send_buf_st.buf == NULL)
    {
#ifdef DEBUG
        trace_printk("L4.5: send buffer set as NULL by cust module, pid %d\n", cust_sock->pid);
#endif
        goto skip_and_send;
    }

    // cust module not required to always mod data, so prevent an unneccesary copy
    // also make sure copy can succeed
    if (cust_sock->send_buf_st.copy_length == 0 || cust_sock->send_buf_st.copy_length > cust_sock->send_buf_st.buf_size)
    {
#ifdef DEBUG
        trace_printk("L4.5 ALERT Copy length problem: copy_length=%lu, buf_size=%u\n",
                     cust_sock->send_buf_st.copy_length, cust_sock->send_buf_st.buf_size);
#endif
        // pretend that we never did anything to the message
        spin_unlock(&cust_sock->active_customization_lock);

#ifdef DEBUG2
        // this is for troubleshooting
        trace_printk("L4.5 ALERT: Copy error, just before return where I think I am giving original buffer to L4\n");
        trace_print_iov_params(&msg->msg_iter);
#endif
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
        trace_printk("L4.5 ALERT: Sendmsg returned error code = %d\n", sendmsg_return);
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
    cust_sock->customize_send_or_skip = SKIP;
    spin_unlock(&cust_sock->active_customization_lock);
    // trace_printk("released lock\n");
    return sendmsg(sk, msg, size);
}




int dca_recvmsg(struct customization_socket *cust_sock, struct sock *sk, struct msghdr *msg, size_t len, int nonblock,
                int flags, int *addr_len, int (*recvmsg)(struct sock *, struct msghdr *, size_t, int, int, int *))
{
    struct customization_node *cust_node;
    struct iov_iter iter_temp;
    size_t copy_success;
    size_t recvmsg_ret;

#ifdef DEBUG1
    trace_printk("L4.5 Start of DCA recvmsg\n");
    trace_print_msg_params(msg);
#endif


    // grab this lock to prevent a cust module from unregistering and creating
    // a NULL pointer problem
    spin_lock(&cust_sock->active_customization_lock);

    cust_node = cust_sock->customization;

    // verify all pointers are good before doing anything else
    if (cust_node == NULL || cust_sock->customize_recv_or_skip == SKIP)
    {
#ifdef DEBUG
        trace_printk("L4.5 ALERT: Cust_node NULL or recv SKIP set unexpectedly, pid %d\n", cust_sock->pid);
#endif
        goto skip_and_error;
    }

    // this case should never happen, but maybe possible if cust removed just before this call
    if (cust_node->recv_function == NULL || cust_sock->recv_buf_st.buf == NULL ||
        cust_sock->recv_buf_st.temp_buf == NULL)
    {
#ifdef DEBUG
        trace_printk("L4.5 ALERT: recv function or buffer set to NULL unexpectedly, pid %d\n", cust_sock->pid);
#endif
        goto skip_and_error;
    }

    // set kmsg to match msg values
    cust_sock->recv_buf_st.kmsg = *msg;
    // setup kmsg with temp buffer
    iov_iter_kvec(&cust_sock->recv_buf_st.kmsg.msg_iter, READ | ITER_KVEC, &cust_sock->recv_buf_st.iov, 1,
                  cust_sock->recv_buf_st.available_bytes);


    // fill the customization buffer instead of msg buffer with desired amount of data to allow customization to process
    // data correctly prior to filling msghdr
    recvmsg_ret =
        recvmsg(sk, &cust_sock->recv_buf_st.kmsg, cust_sock->recv_buf_st.available_bytes, nonblock, flags, addr_len);
    // even if recvmsg_return <= 0, we still want to do cust b/c of the buffering
    // so cust mod now needs to check recvmsg return and decide what to do

#ifdef DEBUG1
    trace_printk("L4.5 Just after kmsg recvmsg\n");
    trace_print_msg_params(&cust_sock->recv_buf_st.kmsg);
#endif


    // this resets the kmsg iter buffer back to point at position before L4 recvmsg b/c module can use iter copy or
    // memcpy
    if (recvmsg_ret > 0)
    {
        iov_iter_revert(&cust_sock->recv_buf_st.kmsg.msg_iter, recvmsg_ret);
    }

    iter_temp = cust_sock->recv_buf_st.kmsg.msg_iter;

    cust_sock->recv_buf_st.copy_length = 0; // just to ensure it is defined
    cust_sock->recv_buf_st.length = len;    // module must know max bytes it can copy
    cust_sock->recv_buf_st.recv_return = recvmsg_ret;

    // passes required data to cust_recv function
    cust_node->recv_function(&cust_sock->recv_buf_st, &cust_sock->socket_flow);

    // restore msg_iter values after cust module may have changed them
    cust_sock->recv_buf_st.kmsg.msg_iter = iter_temp;

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
    if (cust_sock->recv_buf_st.skip_cust == true)
    {
#ifdef DEBUG
        trace_printk("L4.5: cust module skip set, pid %d\n", cust_sock->pid);
#endif
        goto skip_and_recv;
    }


    if (cust_sock->recv_buf_st.no_cust == true)
    {
#ifdef DEBUG
        trace_printk("L4.5: cust module no cust set, pid %d\n", cust_sock->pid);
#endif
        // module didn't do anythinng, so move temp buffer to msg buffer
        // but we check that it can hold that much data in case module messed up
        goto check_and_copy;
    }

    // if cust_sock->recv_buf_st.copy_length = 0 it mean that we deliver a 0 length message to app which basically
    // allows cust to remove all data L4 received
    if (cust_sock->recv_buf_st.copy_length == 0)
    {
        spin_unlock(&cust_sock->active_customization_lock);
        return 0;
    }



    // check if we can successfully perform copy operation
    if (cust_sock->recv_buf_st.copy_length > cust_sock->recv_buf_st.buf_size ||
        cust_sock->recv_buf_st.copy_length > len)
    {
#ifdef DEBUG
        trace_printk("L4.5 ALERT Copy length problem: copy_length=%lu, buf_size=%u, app_len=%lu\n",
                     cust_sock->recv_buf_st.copy_length, cust_sock->recv_buf_st.buf_size, len);
#endif
        // module didn't do correct thing, so move temp buffer to msg buffer
        // but we check that it can hold that much data in case module messed up
        goto check_and_copy;
    }


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
        spin_unlock(&cust_sock->active_customization_lock);
        // TODO: somehow inform module that data not delivered to app
        //  reset iter?
        return -EAGAIN;
    }



#ifdef DEBUG1
    trace_printk("L4.5 End of DCA recvmsg\n");
    trace_print_msg_params(msg);
#endif

#ifdef DEBUG2
    trace_printk("L4.5: After copy success, before return to app\n");
    trace_print_iov_params(&msg->msg_iter);
    trace_print_hex_dump("msg buffer: ", DUMP_PREFIX_ADDRESS, 16, 1, msg->msg_iter.iov->iov_base,
                         cust_sock->recv_buf_st.copy_length, true);
#endif

    // only update time after we recv successfully
    ktime_get_real_ts64(&cust_sock->last_cust_recv_time_struct);
#ifdef DEBUG2
    trace_printk("L4.5: Last recv time: %llu\n", cust_sock->last_cust_recv_time_struct.tv_sec);
#endif

    spin_unlock(&cust_sock->active_customization_lock);

    return cust_sock->recv_buf_st.copy_length;


skip_and_error:
#ifdef DEBUG
    trace_printk("L4.5: skip and error triggered\n");
#endif
    cust_sock->customize_recv_or_skip = SKIP;
    spin_unlock(&cust_sock->active_customization_lock);
    // just make the application ask again
    return -EAGAIN;


skip_and_recv:
#ifdef DEBUG
    trace_printk("L4.5: skip and recv triggered\n");
#endif
    cust_sock->customize_recv_or_skip = SKIP;
    // transfer temp buffer to msg buffer
    copy_success = copy_to_iter(cust_sock->recv_buf_st.temp_buf, recvmsg_ret, &msg->msg_iter);
    spin_unlock(&cust_sock->active_customization_lock);
    // just make the application ask again
    return recvmsg_ret;


check_and_copy:
#ifdef DEBUG
    trace_printk("L4.5: check and copy triggered\n");
#endif
    if (recvmsg_ret > len)
    {
#ifdef DEBUG
        trace_printk("L4.5: recvmsg_ret more than msg can hold, pid %d\n", cust_sock->pid);
#endif
        copy_success = copy_to_iter(cust_sock->recv_buf_st.temp_buf, len, &msg->msg_iter);
        // TODO: should check return here
        spin_unlock(&cust_sock->active_customization_lock);
        return len;
    }
    else
    {
        copy_success = copy_to_iter(cust_sock->recv_buf_st.temp_buf, recvmsg_ret, &msg->msg_iter);
        // TODO: should check return here
        spin_unlock(&cust_sock->active_customization_lock);
        return recvmsg_ret;
    }
}
