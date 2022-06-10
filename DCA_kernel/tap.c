// @file DCA/tap.c
// @brief The Layer 4.5 transport tap functions: UDP, TCPv4

#include <linux/kernel.h>
#include <linux/pid.h>   // For pid_t
#include <linux/sched.h> // For current (pointer to task)
#include <linux/tcp.h>   // For TCP structures
#include <linux/udp.h>   // For UDP structures
#include <net/ip.h>      // for sock structure

#include "customization_socket.h"
#include "send_recv_manager.h"
#include "tap.h"
#include "util/printing.h"

// Update the target application under investigation
#ifdef APP
char TARGET_APP[TASK_NAME_LEN] = "dig"
#endif


    // TCP General reference functions
    int(*ref_tcp_sendmsg)(struct sock * sk, struct msghdr *msg, size_t size);
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
    if (cust_socket != NULL)
    {
        if (cust_socket->customization != NULL)
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
struct sock *new_inet_csk_accept(struct sock *sk, int flags, int *err, bool kern)
{
    struct customization_socket *cust_socket;
    struct sock *new_sock;
    struct task_struct *task = current;

    new_sock = ref_inet_csk_accept(sk, flags, err, kern);
    cust_socket = create_cust_socket(task, new_sock, NULL);
    trace_print_cust_socket(cust_socket, "tcp_accept");
#ifdef DEBUG1
    if (cust_socket != NULL)
    {
        if (cust_socket->customization == NULL)
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
    if (ret == 0)
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
int common_sendmsg(struct sock *sk, struct msghdr *msg, size_t size,
                   int (*sendmsg)(struct sock *, struct msghdr *, size_t))
{
    struct task_struct *task = current;
    struct customization_socket *cust_socket;
    struct task_struct *test = pid_task(find_vpid(task->tgid), PIDTYPE_PID);
    char target_app[TASK_NAME_LEN] = "dig";
    bool found = false;

    // only sending iovec to modules at the moment, but not sure we should prevent
    if (!iter_is_iovec(&msg->msg_iter))
    {
#ifdef DEBUG1
        trace_printk("L4.5: iter type not iovec, pid %d\n", task->pid);
#endif
        return sendmsg(sk, msg, size);
    }

    if (strncmp(test->comm, target_app, TASK_NAME_LEN) == 0)
    {
        trace_printk("L4.5: found app send, pid %d, sk %lu\n", task->pid, (unsigned long)sk);
        found = true;
    }

    cust_socket = get_cust_socket(task, sk);

    // this section generall only runs on first tap attempt (client, UDP)
    if (cust_socket == NULL)
    {
        if (found)
        {
            trace_printk("L4.5: app create cust socket, pid %d, sk %lu\n", task->pid, (unsigned long)sk);
        }
#ifdef DEBUG3
        trace_printk("L4.5: NULL cust socket in sendmsg, creating cust socket for pid %d\n", task->pid);
#endif

        cust_socket = create_cust_socket(task, sk, msg);
        if (cust_socket == NULL)
        {
// Adopt default kernel behavior if cust socket failure
#ifdef DEBUG1
            trace_printk("L4.5: cust_socket NULL not expected, pid %d\n", task->pid);
#endif
            return sendmsg(sk, msg, size);
        }
        if (found)
        {
            trace_printk("L4.5: app cust socket created, pid %d, sk %lu\n", task->pid, (unsigned long)sk);
            trace_print_cust_socket(cust_socket, "target app sendmsg");
        }

#ifdef DEBUG
        else
        {
            if (cust_socket->customization != NULL)
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

    if (found)
    {
        trace_printk("L4.5: cust check, pid %d, sk %lu\n", task->pid, (unsigned long)sk);
        trace_print_cust_socket(cust_socket, "target app sendmsg");
    }

    // if tracking was set to SKIP at some point, stop trying to modify messages
    if (cust_socket->customize_send_or_skip == SKIP)
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
    int peek_performed_already = 0;

#ifdef APP
    struct task_struct *test = pid_task(find_vpid(task->tgid), PIDTYPE_PID);
    bool found = false;
#endif


    // only tapping iovec at the moment, but not sure we should limit
    if (!iter_is_iovec(&msg->msg_iter))
    {
#ifdef DEBUG1
        trace_printk("L4.5: recv iter type not iovec\n");
#endif
        return recvmsg(sk, msg, len, nonblock, flags, addr_len);
    }

#ifdef APP
    if (strncmp(test->comm, TARGET_APP, TASK_NAME_LEN) == 0)
    {
        trace_printk("L4.5: found %s recv, pid %d, sk %lu\n", TARGET_APP, task->pid, (unsigned long)sk);
        found = true;
    }
#endif

    // Try to get cust socket early, but if first time seeing socket this will fail to find the socket
    // Also, if UDP then socket params may hold junk?
    cust_socket = get_cust_socket(task, sk);


    // Perform a PEEK request so we don't remove data from L4 yet, but can fill in missing msg params if they exist
    // perform recvmsg here because nonblock may be false and hold here until L4 has a message to return, which should
    // avoid deadlock on real recvmsg call


    // if cust_socket is NULL, this could be because params were missing
    // so we PEEK and fill them in and try again
    if (cust_socket == NULL)
    {
#ifdef DEBUG1
        trace_printk("L4.5: NULL cust socket before PEEK, pid %d, sk %lu\n", recvmsg_return, task->pid,
                     (unsigned long)sk);
#endif
        recvmsg_return = recvmsg(sk, msg, 0, nonblock, MSG_PEEK, addr_len);
        peek_performed_already = 1;
        // Try again get cust socket now that params may be filled in that were previously junk, but if still first time
        // seeing socket this will fail to find the socket
        cust_socket = get_cust_socket(task, sk);
    }


    // if we found the socket and we have data buffered, then we don't need to do an early recvmsg with PEEK


    if (cust_socket != NULL)
    {
        // buffered data is 0 by default
        if (cust_socket->recv_buf_st.buffered_bytes == 0 && peek_performed_already == 0)
        {
            recvmsg_return = recvmsg(sk, msg, 0, nonblock, MSG_PEEK, addr_len);
        }
    }


    if (recvmsg_return < 0)
    {
#ifdef DEBUG1
        trace_printk("L4.5: Error on recvmsg peek: return value=%d, pid %d, sk %lu\n", recvmsg_return, task->pid,
                     (unsigned long)sk);
#endif
        return recvmsg_return;
    }

    // We need to reset msg->msg_flags b/c TRUNC flag could be set and cause odd application behavior
    // (mainly for UDP; ex: dnsmasq drops message)
    msg->msg_flags = 0;



    // this section generall only runs on first tap attempt (server, UDP)
    if (cust_socket == NULL)
    {
#ifdef DEBUG3
        trace_printk("L4.5: NULL cust socket in recvmsg (after PEEK), creating cust socket for pid %d\n", task->pid);
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
            return -EAGAIN;
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
        if (cust_socket->customization != NULL)
        {
            // Socket added to tracking table, dump socket info to log
            trace_print_cust_socket(cust_socket, "recvmsg");
        }
    // skip is more likely, so we may want to limit logging it
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

    // if tracking was set to SKIP at some point, stop trying to modify messages
    if (cust_socket->customize_recv_or_skip == SKIP)
    {
// most sockets have this set, so only log if really necessary
#ifdef DEBUG3
        trace_printk("L4.5: cust_socket recv set to skip, pid %d\n", task->pid);
#endif

        return recvmsg(sk, msg, len, nonblock, flags, addr_len);
    }

    // at this point we are customizing the socket, so let dca recvmsg handle retrieving message and giving to app
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
