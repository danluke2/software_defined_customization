// @file DCA/tap.c
// @brief The Layer 4.5 transport tap functions: UDP, TCPv4

#include <linux/kernel.h>
#include <linux/sched.h> // For current (pointer to task)
#include <linux/pid.h> // For pid_t
#include <linux/tcp.h> // For TCP structures
#include <linux/udp.h> // For UDP structures
#include <net/ip.h> // for sock structure

#include "tap.h"
#include "util/printing.h"
#include "customization_socket.h"
#include "send_recv_manager.h"





// TCP General reference functions
int (*ref_tcp_sendmsg)(struct sock *sk, struct msghdr *msg, size_t size);
int (*ref_tcp_recvmsg)(struct sock *sk, struct msghdr *msg, size_t len, int nonblock, int flags, int *addr_len);
void (*ref_tcp_close)(struct sock *sk, long timeout);
int (*ref_tcp_v4_connect)(struct sock *sk, struct sockaddr *uaddr, int addr_len);
struct sock *(*ref_inet_csk_accept)(struct sock *sk, int flags, int *err, bool kern);


// TCP General wrapper functions
int new_tcp_sendmsg(struct sock *sk, struct msghdr *msg, size_t size);
int new_tcp_recvmsg(struct sock *sk, struct msghdr *msg, size_t len, int nonblock, int flags, int *addr_len);
void new_tcp_close(struct sock *sk, long timeout);
int new_tcp_v4_connect(struct sock *sk, struct sockaddr *uaddr, int addr_len);
struct sock *new_inet_csk_accept(struct sock *sk, int flags, int *err, bool kern);


// UDP General reference functions
int (*ref_udp_sendmsg)(struct sock *sk, struct msghdr *msg, size_t size);
int (*ref_udp_recvmsg)(struct sock *sk, struct msghdr *msg, size_t len, int nonblock, int flags, int *addr_len);
void (*ref_udp_close)(struct sock *sk, long timeout);


// UDP General wrapper functions
int new_udp_sendmsg(struct sock *sk, struct msghdr *msg, size_t size);
int new_udp_recvmsg(struct sock *sk, struct msghdr *msg, size_t len, int nonblock, int flags, int *addr_len);
void new_udp_close(struct sock *sk, long timeout);


extern struct proto tcp_prot;
extern struct proto udp_prot;


void register_tcp_taps(void)
{
  #ifdef DEBUG1
    trace_printk("L4.5: address of tcp_prot is %p\n", &tcp_prot);
  #endif
	ref_tcp_v4_connect = (void *)tcp_prot.connect;
	ref_tcp_close = (void *)tcp_prot.close;
	ref_tcp_sendmsg = (void *)tcp_prot.sendmsg;
	ref_tcp_recvmsg = (void *)tcp_prot.recvmsg;
	ref_inet_csk_accept = (void *)tcp_prot.accept;

	tcp_prot.connect = new_tcp_v4_connect;
	tcp_prot.close = new_tcp_close;
	tcp_prot.sendmsg = new_tcp_sendmsg;
	tcp_prot.recvmsg = new_tcp_recvmsg;
	tcp_prot.accept = new_inet_csk_accept;

	return;
}


void unregister_tcp_taps(void)
{
	tcp_prot.connect = ref_tcp_v4_connect;
	tcp_prot.close = ref_tcp_close;
	tcp_prot.sendmsg = ref_tcp_sendmsg;
	tcp_prot.recvmsg = ref_tcp_recvmsg;
	tcp_prot.accept = ref_inet_csk_accept;

	return;
}


void register_udp_taps(void)
{
  #ifdef DEBUG1
	  trace_printk("L4.5: address of udp_prot is %p\n", &udp_prot);
  #endif
	ref_udp_close = (void *)udp_prot.close;
	ref_udp_sendmsg = (void *)udp_prot.sendmsg;
	ref_udp_recvmsg = (void *)udp_prot.recvmsg;

	udp_prot.close = new_udp_close;
	udp_prot.sendmsg = new_udp_sendmsg;
	udp_prot.recvmsg = new_udp_recvmsg;

	return;
}


void unregister_udp_taps(void)
{
	udp_prot.close = ref_udp_close;
	udp_prot.sendmsg = ref_udp_sendmsg;
	udp_prot.recvmsg = ref_udp_recvmsg;

	return;
}




// Performs a normal TCPv4 connection, but stores the connection for monitoring.
// Customization socket created to allow TCP connections to do during handshake phase
// @see tcp.h for standard parameters
// @return tcp connect error code
int new_tcp_v4_connect(struct sock *sk, struct sockaddr *uaddr, int addr_len)
{
  int connect_return;
  struct customization_socket *cust_socket;
  struct task_struct *task = current;

  connect_return = ref_tcp_v4_connect(sk, uaddr, addr_len);

  cust_socket = create_cust_socket(task, sk, NULL);
  #ifdef DEBUG
    if(cust_socket != NULL)
    {
      if(cust_socket->customization != NULL)
      {
        // Socket added to tracking table, dump socket info to log
        trace_print_cust_socket(cust_socket, "tcp_connect");
      }
      #ifdef DEBUG1
      else
      {
        trace_printk("L4.5: Customization skipped for pid %d, sk %lu\n", task->pid, (unsigned long)sk);
        // Socket added to tracking table, dump socket info to log
        trace_print_cust_socket(cust_socket, "tcp_connect");
      }
      #endif
    }
  #endif
	return connect_return;
}


// Taps into TCP socket accept and starts tracking socket customization
// if using mptcp, then this may not be best spot to get port info and sock info?
// @see tcp.h: standard parameters
// @see customization_socket.c:create_cust_socket
// @see util/utils.c:print_cust_socket
// @return the requested socket
struct sock* new_inet_csk_accept(struct sock *sk, int flags, int *err, bool kern) {
  struct customization_socket *cust_socket;
  struct sock* new_sock;
  struct task_struct *task = current;

  new_sock = ref_inet_csk_accept(sk, flags, err, kern);
  cust_socket = create_cust_socket(task, new_sock, NULL);
  trace_print_cust_socket(cust_socket, "tcp_accept");
  #ifdef DEBUG1
    if(cust_socket != NULL)
    {
      if(cust_socket->customization == NULL)
      {
        trace_printk("L4.5: Customization skipped for pid %d, sk %lu\n", task->pid, (unsigned long)sk);
      }
    }
  #endif
  return new_sock;
}




// Common code to delete a customization socket from tracking tables
// @see customization_socket.c:delete_cust_socket
void common_close(struct sock *sk)
{
  int ret;
  struct task_struct *task = current;

  ret = delete_cust_socket(task, sk);

  #ifdef DEBUG3
    trace_printk("L4.5: socket close call, pid=%d, sk=%lu\n", task->pid, (unsigned long)sk);
  	if(ret == 0)
    {
      // prints sockets that bypassed L4.5 processing
      trace_printk("L4.5: socket not found; pid=%d, sk=%lu\n", task->pid, (unsigned long)sk);
  	}
  #endif
}


// Runs a TCP close, and closes conneciton monitoring for that connection.
// @see tcp.h for standard parameters
// @return tcp disconnect error code
void new_tcp_close(struct sock *sk, long timeout)
{
  common_close(sk);
	ref_tcp_close(sk, timeout);
	return;
}


// Runs a UDP close and stops Layer 4.5 connection monitoring for the socket
// @see udp.c for standard parameters
void new_udp_close(struct sock *sk, long timeout)
{
  common_close(sk);
	ref_udp_close(sk, timeout);
	return;
}




// Processes sendmsg request and passes to DCA for any changes
// @param sendmsg Pointer to appropriate L4 send function
// @see tcp.h for standard parameters
// @see customization_socket.c:get_cust_socket
// @return the amount of bytes the app wanted to send, or an error code
int common_sendmsg(struct sock *sk, struct msghdr *msg, size_t size, int (*sendmsg)(struct sock*, struct msghdr*, size_t))
{
  struct task_struct *task = current;
  struct customization_socket *cust_socket;

  //only sending iovec to modules at the moment, but not sure we should prevent
  if(!iter_is_iovec(&msg->msg_iter))
  {
    #ifdef DEBUG1
  	 trace_printk("L4.5: iter type not iovec, pid %d\n", task->pid);
    #endif
    return sendmsg(sk, msg, size);
  }

  cust_socket = get_cust_socket(task, sk);

  //this section generall only runs on first tap attempt (client, UDP)
	if(cust_socket == NULL)
  {
    #ifdef DEBUG3
      trace_printk("L4.5: NULL cust socket in recvmsg, creating cust socket for pid %d\n", task->pid);
    #endif

    cust_socket = create_cust_socket(task, sk, msg);
    if(cust_socket == NULL)
    {
      // Adopt default kernel behavior if cust socket failure
      #ifdef DEBUG1
        trace_printk("L4.5: cust_socket NULL not expected, pid %d\n", task->pid);
      #endif
      return sendmsg(sk, msg, size);
    }
    #ifdef DEBUG
      else
      {
        if(cust_socket->customization != NULL)
        {
          trace_printk("L4.5: Customization assigned to pid %d, sk %lu\n", task->pid, (unsigned long)sk);
          // Socket added to tracking table, dump socket info to log
          trace_print_cust_socket(cust_socket, "sendmsg");
        }
        #ifdef DEBUG1
        else
        {
          trace_printk("L4.5: Customization skipped for pid %d, sk %lu\n", task->pid, (unsigned long)sk);
          // Socket added to tracking table, dump socket info to log
          trace_print_cust_socket(cust_socket, "sendmsg");
        }
        #endif
      }
    #endif
	}

  //if tracking was set to SKIP at some point, stop trying to modify messages
  if(cust_socket->customize_send_or_skip == SKIP)
  {
    #ifdef DEBUG3
      trace_printk("L4.5: cust_socket send set to skip, pid %d\n", task->pid);
    #endif
    return sendmsg(sk, msg, size);
  }

  return dca_sendmsg(cust_socket, sk, msg, size, sendmsg);

}


// Taps into tcp_sendmsg and passes to common_sendmsg for processing
// @see tcp.h for standard parameters
int new_tcp_sendmsg(struct sock *sk, struct msghdr *msg, size_t size)
{
  // struct task_struct *task = current;
  // struct task_struct *task_rparent = task->real_parent;
  // struct task_struct *task_parent = task->parent;
  // struct task_struct *test = pid_task(find_vpid(task->tgid), PIDTYPE_PID);
  // trace_printk("L4.5 TCP sendmsg, pid %d, tgid %d, task name = %s\n", task->pid, task->tgid, task->comm);
  // trace_printk("L4.5 TCP sendmsg, real parent pid %d, tgid %d, task name = %s\n", task_rparent->pid, task_rparent->tgid, task_rparent->comm);
  // trace_printk("L4.5 TCP sendmsg, parent pid %d, tgid %d, task name = %s\n", task_parent->pid, task_parent->tgid, task_parent->comm);
  //
  // if(test)
  // {
  //   trace_printk("L4.5 TCP sendmsg, test pid %d, tgid %d, task name = %s\n", test->pid, test->tgid, test->comm);
  // }

  return common_sendmsg(sk, msg, size, ref_tcp_sendmsg);
}


// Taps into udp_sendmsg and passes to common_sendmsg for processing
// @see udp.c for standard parameters
int new_udp_sendmsg(struct sock *sk, struct msghdr *msg, size_t size)
{
  // struct task_struct *task = current;
  // struct task_struct *task_rparent = task->real_parent;
  // struct task_struct *task_parent = task->parent;
  // struct task_struct *test = pid_task(find_vpid(task->tgid), PIDTYPE_PID);
  // trace_printk("L4.5 UDP sendmsg, pid %d, tgid %d, task name = %s\n", task->pid, task->tgid, task->comm);
  // trace_printk("L4.5 UDP sendmsg, real parent pid %d, tgid %d, task name = %s\n", task_rparent->pid, task_rparent->tgid, task_rparent->comm);
  // trace_printk("L4.5 UDP sendmsg, parent pid %d, tgid %d, task name = %s\n", task_parent->pid, task_parent->tgid, task_parent->comm);
  //
  // if(test)
  // {
  //   trace_printk("L4.5 UDP sendmsg, test pid %d, tgid %d, task name = %s\n", test->pid, test->tgid, test->comm);
  // }
  return common_sendmsg(sk, msg, size, ref_udp_sendmsg);
}




// Processes recvmsg request and passes to DCA for any changes
// @param recvmsg Pointer to appropriate L4 recv function
// @see tcp.h for standard parameters
// @see customization_socket.c:get_cust_socket
// @see customization_socket.c:create_cust_socket
// @return the amount of bytes from L4 if no customization performed
// @return the amount of bytes after customization performed, or an error code
int common_recvmsg(struct sock *sk, struct msghdr *msg, size_t len, int nonblock, int flags, int *addr_len, int (*recvmsg)(struct sock*, struct msghdr*, size_t, int, int, int*))
{
  struct task_struct *task = current;
  struct customization_socket *cust_socket;
  int recvmsg_return = 0;


  //only tapping iovec at the moment, but not sure we should limit
  if(!iter_is_iovec(&msg->msg_iter))
  {
    #ifdef DEBUG1
  	 trace_printk("L4.5: recv iter type not iovec\n");
    #endif
    return recvmsg(sk, msg, len, nonblock, flags, addr_len);
  }


  // Try to get cust socket, but if first time seeing socket this will fail to find the socket
  cust_socket = get_cust_socket(task, sk);
  // if(cust_socket == NULL)
  // {
  //   // need to fill the socket params by doing a recvmsg call so we can properly create the cust socket
  //   recvmsg_return = recvmsg(sk, msg, len, nonblock, flags, addr_len);
  //   if(recvmsg_return <= 0)
  //   {
  //     // a 0 length or error message has no need for customization removal
  //     #ifdef DEBUG1
  //       trace_printk("L4.5: received %d length message, pid %d\n", recvmsg_return, task->pid);
  //     #endif
  //     return recvmsg_return;
  //   }
  //   // now we try again to get the cust socket that matches the current socket
  //   cust_socket = get_cust_socket(task, sk);
  // }

  //this section generall only runs on first tap attempt (server, UDP)
  if(cust_socket == NULL)
  {
    #ifdef DEBUG3
      trace_printk("L4.5: NULL cust socket in recvmsg, pid %d\n", task->pid);
    #endif

    // Perform a PEEK request so we don't remove data from L4 yet, but can fill in missing msg params if they exist
    recvmsg_return = recvmsg(sk, msg, 0, nonblock, MSG_PEEK, addr_len);

    cust_socket = create_cust_socket(task, sk, msg);
    if(cust_socket == NULL)
    {
      #ifdef DEBUG1
        trace_printk("L4.5: cust_socket NULL not expected, pid %d\n", task->pid);
      #endif
			return recvmsg_return;
    }
    #ifdef DEBUG
      else
      {
        if(cust_socket->customization != NULL)
        {
          // Socket added to tracking table, dump socket info to log
          trace_print_cust_socket(cust_socket, "recvmsg");
        }
        #ifdef DEBUG1
        else
        {
          trace_printk("L4.5: Customization skipped for pid %d, sk %lu\n", task->pid, (unsigned long)sk);
          // Socket added to tracking table, dump socket info to log
          trace_print_cust_socket(cust_socket, "sendmsg");
        }
        #endif
      }
    #endif
	}

  // if tracking was set to SKIP at some point, stop trying to modify messages
  if(cust_socket->customize_recv_or_skip == SKIP)
  {
    #ifdef DEBUG3
      trace_printk("L4.5: cust_socket recv set to skip, pid %d\n", task->pid);
    #endif
    //undo the PEEK, then do real recvmsg
    // iov_iter_revert(&msg->msg_iter, recvmsg_return);
    // if(recvmsg_return ==0)
    // {
    //   recvmsg_return = recvmsg(sk, msg, len, nonblock, flags, addr_len);
    // }
    // return recvmsg_return;
    return recvmsg(sk, msg, len, nonblock, flags, addr_len);
  }

  // // need to check if kmsg has been allocatted yet
  // if(cust_socket->temp_buf_st.kmsg_ptr == NULL)
  // {
  //   cust_socket->temp_buf_st.kmsg = *msg;
  //   cust_socket->temp_buf_st.kmsg_ptr = msg;
  //   //setup kmsg
  //   iov_iter_kvec(&cust_socket->temp_buf_st.kmsg.msg_iter, READ | ITER_KVEC, &cust_socket->temp_buf_st.iov, 1, cust_socket->temp_buf_st.available_bytes);
  //
  // }
  // // fill the customization buffer instead of msg buffer with desired amount of data to allow customization to process data correctly prior to filling msghdr
  // recvmsg_return = recvmsg(sk, &cust_socket->temp_buf_st.kmsg, cust_socket->temp_buf_st.available_bytes, nonblock, flags, addr_len);
  //
  // // even if recvmsg_return = 0, we still want to do cust b/c of the buffering
  // if(recvmsg_return < 0)
  // {
  //   // an error message has no need for customization removal
  //   #ifdef DEBUG1
  //     trace_printk("L4.5: received %d length message, pid %d\n", recvmsg_return, task->pid);
  //   #endif
  //   return recvmsg_return;
  // }

  return dca_recvmsg(cust_socket, sk, msg, len, nonblock, flags, addr_len, recvmsg);
}


// Taps into tcp_recvmsg and passes to common_sendmsg for processing
// @see tcp.h for standard parameters
int new_tcp_recvmsg(struct sock *sk, struct msghdr *msg, size_t len, int nonblock, int flags, int *addr_len)
{
  // struct task_struct *task = current;
  // struct task_struct *task_rparent = task->real_parent;
  // struct task_struct *task_parent = task->parent;
  // struct task_struct *test = pid_task(find_vpid(task->tgid), PIDTYPE_PID);
  // trace_printk("L4.5 TCP recvmsg, pid %d, tgid %d, task name = %s\n", task->pid, task->tgid, task->comm);
  // trace_printk("L4.5 TCP recvmsg, real parent pid %d, tgid %d, task name = %s\n", task_rparent->pid, task_rparent->tgid, task_rparent->comm);
  // trace_printk("L4.5 TCP recvmsg, parent pid %d, tgid %d, task name = %s\n", task_parent->pid, task_parent->tgid, task_parent->comm);
  // if(test)
  // {
  //   trace_printk("L4.5 TCP recvmsg, test pid %d, tgid %d, task name = %s\n", test->pid, test->tgid, test->comm);
  // }
  return common_recvmsg(sk, msg, len, nonblock, flags, addr_len, ref_tcp_recvmsg);
}


// Taps into udp_recvmsg and passes to common_sendmsg for processing
// @see udp.c for standard parameters
int new_udp_recvmsg(struct sock *sk, struct msghdr *msg, size_t len, int nonblock, int flags, int *addr_len)
{
  return common_recvmsg(sk, msg, len, nonblock, flags, addr_len, ref_udp_recvmsg);
}
