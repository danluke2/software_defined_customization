// @file DCA/tap.c
// @brief The Layer 4.5 transport tap functions: UDP, TCPv4

#include <linux/kernel.h>
#include <linux/pid.h>   // For pid_t
#include <linux/sched.h> // For current (pointer to task)
#include <linux/socket.h>
#include <linux/tcp.h> // For TCP structures
#include <linux/udp.h> // For UDP structures
#include <net/ip.h>    // for sock structure

#include "customization_socket.h"
#include "send_recv_manager.h"
#include "tap.h"
#include "util/helpers.h"

// Update the target application under investigation
#ifdef APP
char TARGET_APP[TASK_NAME_LEN] = "dig";
#endif


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



//TODO: Consider adding data to the TCP connect to support things like secure vector routing (SVR)
//NOTE: This may be better suited as a BPF function at tc or by using BPF paper to wrap in UDP

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

    //TODO: should check connect_return succeeded before building cust socket
    cust_socket = create_cust_socket(task, sk, NULL);
#ifdef DEBUG
    if (cust_socket != NULL)
    {
        //TODO: Why posit 0 instead of a global name?
        if (cust_socket->customizations[0] != NULL)
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


//TODO: if using mptcp, then this may not be best spot to get port info and sock info?
//TODO: if adding syn data, would this be the place to add syn/ack data?

// Taps into TCP socket accept and starts tracking socket customization
// @see tcp.h: standard parameters
// @see customization_socket.c:create_cust_socket
// @see util/utils.c:print_cust_socket
// @return the requested socket
struct sock *new_inet_csk_accept(struct sock *sk, int flags, int *err, bool kern)
{
    struct customization_socket *cust_socket;
    struct sock *new_sock;
    struct task_struct *task = current;

    new_sock = ref_inet_csk_accept(sk, flags, err, kern);

    //TODO: should check new_sock succeeded before building cust socket
    cust_socket = create_cust_socket(task, new_sock, NULL);
#ifdef DEBUG
    if (cust_socket != NULL)
    {
        if (cust_socket->customizations[0] != NULL)
        {
            // Socket added to tracking table, dump socket info to log
            trace_print_cust_socket(cust_socket, "tcp_accept");
        }
    #ifdef DEBUG1
        else
        {
            trace_printk("L4.5: Customization skipped for pid %d, sk %lu\n", task->pid, (unsigned long)sk);
            // Socket added to tracking table, dump socket info to log
            trace_print_cust_socket(cust_socket, "tcp_accept");
        }
    #endif
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
    if (ret == 0)
    {
        // prints sockets that were not processedy by L4.5
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


// TODO: Would it be useful to send a UDP message to indicate we are closing the socket?

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
int common_sendmsg(struct sock *sk, struct msghdr *msg, size_t size,
                   int (*sendmsg)(struct sock *, struct msghdr *, size_t))
{
    struct task_struct *task = current;
    struct customization_socket *cust_socket;

    // only sending iovec to modules at the moment, but not sure we should prevent
    if (!iter_is_iovec(&msg->msg_iter))
    {
#ifdef DEBUG1
        trace_printk("L4.5: iter type not iovec, pid %d\n", task->pid);
#endif
        return sendmsg(sk, msg, size);
    }

    cust_socket = get_cust_socket(task, sk);

    // this section generally only runs on first tap attempt (client, UDP)
    if (cust_socket == NULL)
    {
#ifdef DEBUG3
        trace_printk("L4.5: NULL cust socket in sendmsg, creating cust socket for pid %d\n", task->pid);
#endif

        cust_socket = create_cust_socket(task, sk, msg);
        if (cust_socket == NULL)
        {
#ifdef DEBUG1
            trace_printk("L4.5: cust_socket NULL not expected, pid %d\n", task->pid);
#endif
            // Adopt default kernel behavior if cust socket failure
            return sendmsg(sk, msg, size);
        }
#ifdef DEBUG
        else
        {
            if (cust_socket->customizations[0] != NULL)
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

    // if update check set due to new cust module, then need to check if socket should be customized now
    if (cust_socket->update_cust_check)
    {
#ifdef DEBUG3
        trace_printk("L4.5: cust recheck triggered, pid %d\n", task->pid);
#endif
        update_cust_status(cust_socket, task, sk);
    }


    // if tracking was set to SKIP at some point, stop trying to modify messages
    if (cust_socket->customize_or_skip == SKIP)
    {
#ifdef DEBUG3
        trace_printk("L4.5: cust_socket send set to skip, pid %d\n", task->pid);
#endif
        return sendmsg(sk, msg, size);
    }



    return dca_sendmsg(cust_socket, sk, msg, size, sendmsg);
}


//TODO: Is it better to call common_sendmsg without passing ref_xxx_sendmsg as a param?  
// Would return to new_xxx_sendmsg instead, which could either send normal or call the dca
// Also, should dca return here before sending?  This may be complicated though

// Taps into tcp_sendmsg and passes to common_sendmsg for processing
// @see tcp.h for standard parameters
int new_tcp_sendmsg(struct sock *sk, struct msghdr *msg, size_t size)
{
    //TODO: This should be wrapped in an appropriate debug block

    // struct task_struct *task = current;
    // struct task_struct *task_rparent = task->real_parent;
    // struct task_struct *task_parent = task->parent;
    // struct task_struct *test = pid_task(find_vpid(task->tgid), PIDTYPE_PID);
    // trace_printk("L4.5 TCP sendmsg, pid %d, tgid %d, task name = %s\n", task->pid, task->tgid, task->comm);
    // trace_printk("L4.5 TCP sendmsg, real parent pid %d, tgid %d, task name = %s\n", task_rparent->pid,
    // task_rparent->tgid, task_rparent->comm); trace_printk("L4.5 TCP sendmsg, parent pid %d, tgid %d, task name =
    // %s\n", task_parent->pid, task_parent->tgid, task_parent->comm);
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
    // trace_printk("L4.5 UDP sendmsg, real parent pid %d, tgid %d, task name = %s\n", task_rparent->pid,
    // task_rparent->tgid, task_rparent->comm); trace_printk("L4.5 UDP sendmsg, parent pid %d, tgid %d, task name =
    // %s\n", task_parent->pid, task_parent->tgid, task_parent->comm);
    //
    // if(test)
    // {
    //   trace_printk("L4.5 UDP sendmsg, test pid %d, tgid %d, task name = %s\n", test->pid, test->tgid, test->comm);
    // }
    return common_sendmsg(sk, msg, size, ref_udp_sendmsg);
}



//TODO: I beleive this needs to be updated from the buffer branch still

// Processes recvmsg request and passes to DCA for any changes
// @param recvmsg Pointer to appropriate L4 recv function
// @see tcp.h for standard parameters
// @see customization_socket.c:get_cust_socket
// @see customization_socket.c:create_cust_socket
// @return the amount of bytes from L4 if no customization performed
// @return the amount of bytes after customization performed, or an error code
int common_recvmsg(struct sock *sk, struct msghdr *msg, size_t len, int nonblock, int flags, int *addr_len,
                   int (*recvmsg)(struct sock *, struct msghdr *, size_t, int, int, int *))
{
    struct task_struct *task = current;
    struct customization_socket *cust_socket;
    int recvmsg_return = 0;
#ifdef APP
    struct task_struct *test = pid_task(find_vpid(task->tgid), PIDTYPE_PID);
    bool found = false;
#endif


    // TODO: only tapping iovec at the moment, but not sure we should limit
    if (!iter_is_iovec(&msg->msg_iter))
    {
#ifdef DEBUG1
        trace_printk("L4.5: recv iter type not iovec\n");
#endif
        return recvmsg(sk, msg, len, nonblock, flags, addr_len);
    }

//NOTE: This is used for targeting a specific app during testing, so probably better as a new debug type
#ifdef APP
    if (strncmp(test->comm, TARGET_APP, TASK_NAME_LEN) == 0)
    {
        trace_printk("L4.5: found %s recv, pid %d, sk %lu\n", TARGET_APP, task->pid, (unsigned long)sk);
        found = true;
    }
#endif

    recvmsg_return = recvmsg(sk, msg, len, nonblock, flags, addr_len);
    // TODO: Should we let these messages go through cust module in case it wants to add data?
    if (recvmsg_return <= 0)
    {
// a 0 length or error message has no need for customization removal
#ifdef DEBUG1
        trace_printk("L4.5: received %d length message, pid %d\n", recvmsg_return, task->pid);
#endif
        return recvmsg_return;
    }

    cust_socket = get_cust_socket(task, sk);

    // this section generall only runs on first tap attempt (server, UDP)
    if (cust_socket == NULL)
    {
#ifdef DEBUG3
        trace_printk("L4.5: NULL cust socket in recvmsg, creating cust socket for pid %d\n", task->pid);
#endif

#ifdef APP
        if (found)
        {
            trace_printk("L4.5: %s create cust socket, pid %d, sk %lu\n", TARGET_APP, task->pid, (unsigned long)sk);
        }
#endif

        cust_socket = create_cust_socket(task, sk, msg);
        if (cust_socket == NULL)
        {
#ifdef DEBUG1
            trace_printk("L4.5: cust_socket NULL not expected, pid %d\n", task->pid);
#endif
            return recvmsg_return;
        }

        // allow logging for TARGET_APP regardless of customization status
#ifdef APP
        if (found)
        {
            trace_printk("L4.5: %s cust socket created, pid %d, sk %lu\n", TARGET_APP, task->pid, (unsigned long)sk);
            trace_printk("L4.5: recvmsg was %d, pid %d, sk %lu\n", recvmsg_return, task->pid, (unsigned long)sk);
            trace_print_cust_socket(cust_socket, "target app recvmsg");
        }
#endif

#ifdef DEBUG
        if (cust_socket->customizations[0] != NULL)
        {
            // Socket added to tracking table, dump socket info to log
            trace_print_cust_socket(cust_socket, "recvmsg");
        }
    #ifdef DEBUG1
        else
        {
            trace_printk("L4.5: Customization skipped for pid %d, sk %lu\n", task->pid, (unsigned long)sk);
            // Socket added to tracking table, dump socket info to log
            trace_print_cust_socket(cust_socket, "recvmsg");
        }
    #endif
#endif
    }

    // if update check set due to new cust module, then need to check if socket should be customized now
    if (cust_socket->update_cust_check)
    {
#ifdef DEBUG3
        trace_printk("L4.5: cust recheck triggered, pid %d\n", task->pid);
#endif
        update_cust_status(cust_socket, task, sk);
    }

    // if tracking was set to SKIP at some point, stop trying to modify messages
    if (cust_socket->customize_or_skip == SKIP)
    {
#ifdef DEBUG3
        trace_printk("L4.5: cust_socket recv set to skip, pid %d\n", task->pid);
#endif
        return recvmsg_return;
    }

    return dca_recvmsg(cust_socket, sk, msg, len, recvmsg_return);
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
    // trace_printk("L4.5 TCP recvmsg, real parent pid %d, tgid %d, task name = %s\n", task_rparent->pid,
    // task_rparent->tgid, task_rparent->comm); trace_printk("L4.5 TCP recvmsg, parent pid %d, tgid %d, task name =
    // %s\n", task_parent->pid, task_parent->tgid, task_parent->comm); if(test)
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
