// @file send_recv_manager.c
// @brief The functions to perform send and recv calls using customization modules

#include <linux/sched.h>
#include <linux/timekeeping.h> // for timestamps

#include "send_recv_manager.h"
#include "util/printing.h"


int dca_sendmsg(struct customization_socket *cust_sock, struct sock *sk, struct msghdr *msg, size_t size,
								int (*sendmsg)(struct sock*, struct msghdr*, size_t))
{
  size_t copy_length = 0;
  struct msghdr kmsg = *msg;
  struct kvec iov;
  struct customization_node *cust_node;
  int sendmsg_return;

	#ifdef DEBUG2
		trace_printk("L4.5: Start of DCA sendmsg, given size = %lu\n", size);
		trace_print_msg_params(msg);
	#endif

	spin_lock(&cust_sock->active_customization_lock);
	cust_node = cust_sock->customization;
	if(cust_node == NULL || cust_sock->customize_send_or_skip == SKIP)
	{
		// customization could have been unregistered right before this call
		#ifdef DEBUG1
      trace_printk("L4.5 ALERT: Cust_node NULL or send SKIP set unexpectedly, pid %d\n", cust_sock->pid);
    #endif
		goto skip_and_send;
	}

	// this case should never happen, but checking to ensure no future errors
	if(cust_node->send_function == NULL || cust_sock->send_buf_st.buf == NULL)
	{
		#ifdef DEBUG1
      trace_printk("L4.5 ALERT: send function or buffer set to NULL unexpectedly, pid %d\n", cust_sock->pid);
    #endif
		goto skip_and_send;
	}

	// store msg_iter values before cust modules does its thing
	cust_sock->send_buf_st.iter_copy.iov_offset = msg->msg_iter.iov_offset;
	cust_sock->send_buf_st.iter_copy.count = msg->msg_iter.count;
	cust_sock->send_buf_st.iter_copy.nr_segs = msg->msg_iter.nr_segs;
	cust_sock->send_buf_st.iter_copy.iov = msg->msg_iter.iov;

  // passes msg iter to customization send function
  cust_node->send_function(&msg->msg_iter, &cust_sock->send_buf_st, size, &copy_length);

	// restore msg_iter values after cust module likely changed them
	msg->msg_iter.iov_offset = cust_sock->send_buf_st.iter_copy.iov_offset;
	msg->msg_iter.count = cust_sock->send_buf_st.iter_copy.count;
	msg->msg_iter.nr_segs = cust_sock->send_buf_st.iter_copy.nr_segs;
	msg->msg_iter.iov = cust_sock->send_buf_st.iter_copy.iov;

	#ifdef DEBUG2
		// this is for troubleshooting; iter should be at state given to send
		trace_printk("L4.5: After cust send module return\n");
		trace_print_iov_params(&msg->msg_iter);
	#endif

	//TODO: handle case when cust mod wants to trigger send failure?

  // cust module must have valid buffer pointer
	// modules could set to NULL to signal no longer customizing the socket
  if(cust_sock->send_buf_st.buf == NULL)
  {
		#ifdef DEBUG1
      trace_printk("L4.5: send buffer set as NULL by cust module, pid %d\n", cust_sock->pid);
    #endif
    goto skip_and_send;
  }

  // cust module not required to always mod data, so prevent an unneccesary copy
  // also make sure copy can succeed
  if(copy_length==0 || copy_length > cust_sock->send_buf_st.buf_size)
  {
		#ifdef DEBUG
			trace_printk("L4.5 ALERT Copy length problem: copy_length=%lu, buf_size=%u\n", copy_length, cust_sock->send_buf_st.buf_size);
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
		trace_printk("L4.5: Module adjusted send size = %lu\n", copy_length);
	#endif

	//create new iter based on cust send buffer to give to L4
	iov.iov_base = cust_sock->send_buf_st.buf;
  iov.iov_len = copy_length;
	// this needs to be kvec since we are in kernel space; 1 = number of segments
	iov_iter_kvec(&kmsg.msg_iter, READ, &iov, 1, copy_length);

	sendmsg_return = sendmsg(sk, &kmsg, copy_length);
	spin_unlock(&cust_sock->active_customization_lock);

	if(sendmsg_return<0)
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




int dca_recvmsg(struct customization_socket *cust_sock, struct sock *sk,
								struct msghdr *msg, size_t len, size_t recvmsg_ret)
{
	struct customization_node *cust_node;
	size_t copy_length = len; // just to ensure it is defined
  size_t copy_success;

	#ifdef DEBUG2
		trace_printk("L4.5 Start of DCA recvmsg\n");
		trace_print_msg_params(msg);
	#endif

	// this resets the msg iter buffer back to point at positions before L4 recvmsg
	// put data into the buffer; NOTE: recvmsg_ret is > 0
	iov_iter_revert(&msg->msg_iter, recvmsg_ret);

	#ifdef DEBUG2
		trace_printk("L4.5: Iter after reverting %lu bytes\n", recvmsg_ret);
		trace_print_iov_params(&msg->msg_iter);
	#endif

  // grab this lock to prevent a cust module from unregistering and creating
  // a NULL pointer problem
	spin_lock(&cust_sock->active_customization_lock);
	// trace_printk("have lock\n");
	cust_node = cust_sock->customization;

	if(cust_node == NULL || cust_sock->customize_recv_or_skip == SKIP)
	{
		#ifdef DEBUG1
      trace_printk("L4.5 ALERT: Cust_node NULL or recv SKIP set unexpectedly, pid %d\n", cust_sock->pid);
    #endif
		goto skip_and_recv;
	}

	// this case should never happen, but possible if cust removed just before this call
	if(cust_node->recv_function == NULL || cust_sock->recv_buf_st.buf == NULL)
	{
		#ifdef DEBUG1
      trace_printk("L4.5 ALERT: recv function or buffer set to NULL unexpectedly, pid %d\n", cust_sock->pid);
    #endif
		goto skip_and_recv;
	}

	// store msg_iter values before cust modules does its thing
	cust_sock->recv_buf_st.iter_copy.iov_offset = msg->msg_iter.iov_offset;
	cust_sock->recv_buf_st.iter_copy.count = msg->msg_iter.count;
	cust_sock->recv_buf_st.iter_copy.nr_segs = msg->msg_iter.nr_segs;
	cust_sock->recv_buf_st.iter_copy.iov = msg->msg_iter.iov;

	// passes msg iter to customization recv function
  cust_node->recv_function(&msg->msg_iter, &cust_sock->recv_buf_st, len, recvmsg_ret, &copy_length);

	// restore msg_iter values after cust module likely changed them
	msg->msg_iter.iov_offset = cust_sock->recv_buf_st.iter_copy.iov_offset;
	msg->msg_iter.count = cust_sock->recv_buf_st.iter_copy.count;
	msg->msg_iter.nr_segs = cust_sock->recv_buf_st.iter_copy.nr_segs;
	msg->msg_iter.iov = cust_sock->recv_buf_st.iter_copy.iov;

	#ifdef DEBUG2
		trace_printk("L4.5: After cust module return\n");
		trace_print_iov_params(&msg->msg_iter);
	#endif

	// cust module must have valid buffer pointer
	// modules could set to NULL to signal no longer customizing the socket
  if(cust_sock->recv_buf_st.buf == NULL)
  {
		#ifdef DEBUG1
      trace_printk("L4.5: send buffer set as NULL by cust module, pid %d\n", cust_sock->pid);
    #endif
    goto skip_and_recv;
  }

	//TODO: handle case when cust mod wants to trigger recv failure?

	// TODO: copy_length = 0 should mean that we deliver a 0 length message to app
	// which basically allows cust to remove all data L4 received

  // cust module not required to always mod data, so prevent an unneccesary copy
  // also make sure copy can succeed
  if(copy_length == 0 || copy_length > cust_sock->recv_buf_st.buf_size || copy_length > len)
  {
		#ifdef DEBUG
    	trace_printk("L4.5 ALERT Copy length problem: copy_length=%lu, buf_size=%u, app_len=%lu\n", copy_length, cust_sock->recv_buf_st.buf_size, len);
		#endif
		// pretend that we never did anything to the message
		iov_iter_advance(&msg->msg_iter, recvmsg_ret);
		spin_unlock(&cust_sock->active_customization_lock);

		#ifdef DEBUG2
			trace_printk("L4.5 ALERT Copy error, just before return where I think I am giving original buffer back\n");
			trace_print_iov_params(&msg->msg_iter);
		#endif
    return recvmsg_ret;
  }

  #ifdef DEBUG2
		trace_printk("L4.5 just before copy_to_iter, desired bytes = %lu\n", copy_length);
		trace_print_iov_params(&msg->msg_iter);
	#endif

  copy_success = copy_to_iter(cust_sock->recv_buf_st.buf, copy_length, &msg->msg_iter);

	spin_unlock(&cust_sock->active_customization_lock);

	//I don't think this is likely, but need to check
  if(copy_success != copy_length)
  {
		#ifdef DEBUG
    	trace_printk("L4.5 ALERT Copied bytes not same as desired value, copied = %lu, desired = %lu\n", copy_success, copy_length);
			#ifdef DEBUG2
				trace_print_iov_params(&msg->msg_iter);
			#endif
		#endif
    //TODO: somehow inform module that data not delivered to app
		// reset iter?
    return -EAGAIN;

    //Scenario 2: stop trying to customize this socket
    // free cust buffer
		// reset iter?
    // return -EAGAIN;
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

  return copy_length;


skip_and_recv:
	#ifdef DEBUG
		trace_printk("L4.5: skip and recv triggered\n");
		#ifdef DEBUG2
			trace_print_iov_params(&msg->msg_iter);
		#endif
	#endif
	cust_sock->customize_recv_or_skip = SKIP;
	iov_iter_advance(&msg->msg_iter, recvmsg_ret);
	spin_unlock(&cust_sock->active_customization_lock);
	return recvmsg_ret;
}