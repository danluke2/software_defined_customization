#ifndef _CUSTOMIZATION_STRUCTS_H
#define _CUSTOMIZATION_STRUCTS_H

#include <linux/ktime.h>
#include <linux/list.h>
#include <linux/time64.h>
#include <linux/timekeeping.h>
#include <net/sock.h>



#define TASK_NAME_LEN 16 // defined in sched.h as TASK_COMM_LEN 16
#define SEND_BUF_SIZE 65536
#define RECV_BUF_SIZE 65536

// for functions that need to know direction of messages
#define SEND 0
#define RECV 1


// for modules challenge_response support
#define AES_BLOCK_SIZE 16
#define SYMMETRIC_KEY_LENGTH 32
#define HEX_KEY_LENGTH 64
#define IV_LENGTH 16
#define HEX_IV_LENGTH 32
#define CHALLENGE_LENGTH 16
#define HEX_CHALLENGE_LENGTH 32
#define RESPONSE_LENGTH 32
#define HEX_RESPONSE_LENGTH 64



enum customization_state
{
    SKIP,
    CUSTOMIZE,
    UNKNOWN // signal to check again for new cust modules
};


struct customization_flow
{
    u16 dest_port;
    u16 source_port;
    __be32 dest_ip;
    __be32 source_ip;
    u16 protocol;
    // pid is the thred id and will match thread names like python3, isc-worker, etc.
    char task_name_pid[TASK_NAME_LEN];
    // tgid is the thread group ID and will match apps like dig, firefox, etc.
    char task_name_tgid[TASK_NAME_LEN];

    // these may be used to match subnets at some point
    // u8 dest_mask;
    // u8 src_mask;
};


// buffers for customization module use; send and recv have own buffers
struct customization_buffer
{
    void *buf; // malloc only when assigned a customization
    u32 buf_size;
    size_t copy_length;        // how much of buffer to copy
    struct iov_iter *src_iter; // source buffer to work from
    size_t length;             // send=amount of data in src_iter, recv=max amount to return
    size_t recv_return;        // amount of data L4 returned from recvmsg call

    bool skip_cust;
    bool no_cust;
};


// primary structure for application sockets to hold customization information
struct customization_socket
{
    pid_t pid;
    pid_t tgid;
    struct sock *sk;
    uid_t uid; // up to modules to map ID to username if desired

    // customization can be one way, so allow for send/recv differentiation
    enum customization_state customize_send_or_skip;
    enum customization_state customize_recv_or_skip;

    struct customization_flow socket_flow;

    // custimation will be cast to customization node
    void *customization;
    struct customization_buffer send_buf_st;
    struct customization_buffer recv_buf_st;
    struct timespec64 last_cust_send_time_struct; // last time cust was applied
    struct timespec64 last_cust_recv_time_struct; // last time cust was applied

    // lock to prevent cust removal while actively using
    spinlock_t active_customization_lock;

    unsigned long hash_key;
    struct hlist_node socket_hash;
};


// structure to hold customization info to apply to a customization socket
struct customization_node
{
    struct customization_flow target_flow;

    // mod_id
    u16 cust_id;
    u16 *standby_mode;
    // counter to track number of sockets cust is applied to
    u16 sock_count;

    // cust can set these to override the defualt SEND_BUF_SIZE, RECV_BUF_SIZE
    u32 send_buffer_size;
    u32 recv_buffer_size;

    struct timespec64 registration_time_struct;
    struct timespec64 deprecated_time_struct;
    struct timespec64 revoked_time_struct;

    void (*send_function)(struct customization_buffer *send_buf_st, struct customization_flow *socket_flow);

    void (*recv_function)(struct customization_buffer *recv_buf_st, struct customization_flow *socket_flow);

    // challenge function called when DCA issues a module challenge-response request
    void (*challenge_function)(char *response_buffer, char *iv, char *challenge_message);

    struct list_head cust_list_member;
    struct list_head deprecated_cust_list_member;
    struct list_head revoked_cust_list_member;
};


#endif
