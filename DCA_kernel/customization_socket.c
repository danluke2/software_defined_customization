// @file customization_socket.c
// @brief The functions to establish socket customization at Layer 4.5


#include <linux/hashtable.h>
#include <linux/slab.h>
#include <linux/socket.h>
#include <linux/sort.h>
#include <linux/timekeeping.h> // for timestamps
#include <linux/uio.h>         // For iovec structures
#include <net/inet_sock.h>

#include "customization_socket.h"
#include "register_cust.h" //for get_customization
#include "util/compare.h"
#include "util/printing.h"
#include <linux/cred.h> //uid testing


unsigned int socket_allocsminusfrees;
#define HASH_TABLE_BITSIZE 10

static DEFINE_HASHTABLE(cust_socket_table, HASH_TABLE_BITSIZE);
static DEFINE_HASHTABLE(normal_socket_table, HASH_TABLE_BITSIZE);


static DEFINE_SPINLOCK(cust_socket_lock);
static DEFINE_SPINLOCK(normal_socket_lock);


// helpers
static void set_four_tuple(struct sock *sk, struct customization_socket *cust_socket, struct msghdr *msg);

static void assign_customizations(struct customization_socket *cust_sock, struct customization_node *cust_node[],
                                  size_t node_counter);

static size_t assign_customization_to_array(struct customization_socket *cust_sock,
                                            struct customization_node *cust_node);

static void unassign_all_customization(struct customization_socket *cust_sock);
static void unassign_single_customization(struct customization_socket *cust_sock, struct customization_node *cust_node);




void init_socket_tables(void)
{
    socket_allocsminusfrees = 0;
    hash_init(cust_socket_table);
    hash_init(normal_socket_table);
    return;
}




struct customization_socket *create_cust_socket(struct task_struct *task, struct sock *sk, struct msghdr *msg)
{
    struct customization_socket *new_cust_socket = NULL;
    struct customization_node *cust_nodes[MAX_CUST_ATTACH] = {NULL};
    struct task_struct *tgid_task = pid_task(find_vpid(task->tgid), PIDTYPE_PID);
    size_t node_counter = 0;
    size_t i;

    new_cust_socket = kmalloc(sizeof(struct customization_socket), GFP_KERNEL);
    if (new_cust_socket == NULL)
    {
#ifdef DEBUG
        trace_printk("kmalloc failed when creating customization socket\n");
#endif
        return NULL;
    }
    socket_allocsminusfrees++;


    new_cust_socket->pid = task->pid;   // likely 4 bytes long (depends on system)
    new_cust_socket->tgid = task->tgid; // this might be same as pid
    new_cust_socket->sk = sk;
    new_cust_socket->hash_key = task->pid ^ (unsigned long)sk;
    new_cust_socket->uid = get_current_user()->uid.val;
    new_cust_socket->socket_flow.protocol = sk->sk_protocol;
    memcpy(new_cust_socket->socket_flow.task_name_pid, task->comm, TASK_NAME_LEN);
    memcpy(new_cust_socket->socket_flow.task_name_tgid, tgid_task->comm, TASK_NAME_LEN);

    // default pointer values to prevent random errors
    for (i = 0; i < MAX_CUST_ATTACH; i++)
    {
        // put NULL pointers into customization array
        new_cust_socket->customizations[i] = NULL;
    }
    new_cust_socket->send_buf_st.buf = NULL;
    new_cust_socket->send_buf_st.no_cust = false;
    new_cust_socket->send_buf_st.set_cust_to_skip = false;
    new_cust_socket->send_buf_st.try_next = false;

    new_cust_socket->recv_buf_st.buf = NULL;
    new_cust_socket->recv_buf_st.no_cust = false;
    new_cust_socket->recv_buf_st.set_cust_to_skip = false;
    new_cust_socket->recv_buf_st.try_next = false;

    // set default time values to indicate no packets seen yet
    new_cust_socket->last_cust_send_time_struct.tv_sec = 0;
    new_cust_socket->last_cust_send_time_struct.tv_nsec = 0;
    new_cust_socket->last_cust_recv_time_struct.tv_sec = 0;
    new_cust_socket->last_cust_recv_time_struct.tv_nsec = 0;

    set_four_tuple(sk, new_cust_socket, msg);

    // all necessary values set to match a customization
    node_counter = get_customizations(new_cust_socket, cust_nodes);
    if (node_counter == 0)
    {
#ifdef DEBUG3
        trace_printk("L4.5: cust request lookup NULL, proto=%u, pid task=%s, tgid task=%s, uid = %d\n", sk->sk_protocol,
                     task->comm, tgid_task->comm, new_cust_socket->uid);
#endif
        new_cust_socket->customize_or_skip = SKIP;
    }
    else
    {
#ifdef DEBUG
        trace_printk("L4.5: Assigning cust to socket, pid %d\n", task->pid);
#endif
        assign_customizations(new_cust_socket, cust_nodes, node_counter);
    }

    // fist customization array posit must hold a cust node to be valid
    if (new_cust_socket->customizations[0] == NULL)
    {
#ifdef DEBUG2
        trace_printk("L4.5: Adding pid %d to normal table\n", task->pid);
#endif
        spin_lock(&normal_socket_lock);
        hash_add(normal_socket_table, &new_cust_socket->socket_hash, new_cust_socket->hash_key);
        spin_unlock(&normal_socket_lock);
    }
    else
    {
#ifdef DEBUG2
        trace_printk("L4.5: Adding pid %d to customization table\n", task->pid);
#endif
        spin_lock(&cust_socket_lock);
        hash_add(cust_socket_table, &new_cust_socket->socket_hash, new_cust_socket->hash_key);
        spin_unlock(&cust_socket_lock);
    }
    return new_cust_socket;
}




void update_cust_status(struct customization_socket *cust_socket, struct task_struct *task, struct sock *sk)
{
    struct customization_node *cust_nodes[MAX_CUST_ATTACH] = {NULL};
    size_t node_counter = 0;
    size_t assign_counter = 0;
    int i;
    int j;
    bool found = false;

    // 2 cases: add cust to non-cust socket, add cust to cust socket that has room for new cust

    node_counter = get_customizations(cust_socket, cust_nodes);
    if (node_counter == 0)
    {
#ifdef DEBUG3
        trace_printk("L4.5: cust update lookup NULL, proto=%u, pid task=%s, tgid task=%s, uid = %d\n",
                     cust_socket->socket_flow.protocol, cust_socket->socket_flow.task_name_pid,
                     cust_socket->socket_flow.task_name_tgid, cust_socket->uid);
#endif
        cust_socket->customize_or_skip = SKIP;
        return;
    }

    // case 1: previously skipped socket is now customized
    if (cust_socket->customizations[0] == NULL)
    {
#ifdef DEBUG
        trace_printk("L4.5: Assigning cust to socket, pid %d\n", task->pid);
#endif
        assign_customizations(cust_socket, cust_nodes, node_counter);


        // if we are now customizing the socket, we must move the socket to cust table
        if (cust_socket->customizations[0] != NULL)
        {
#ifdef DEBUG2
            trace_printk("L4.5: Adding pid %d to customization table\n", task->pid);
#endif
            // remove from normal table
            spin_lock(&normal_socket_lock);

            hash_del(&cust_socket->socket_hash);

            // add to cust table before releasing normal lock?
            spin_lock(&cust_socket_lock);
            hash_add(cust_socket_table, &cust_socket->socket_hash, cust_socket->hash_key);
            spin_unlock(&cust_socket_lock);

            spin_unlock(&normal_socket_lock);
        }
    }
    // case 2: we are adjusting modules attached to cust socket
    // loop through the current list of modules and add new modules where NULL pointers are
    else
    {
        for (i = 0; i < node_counter; i++)
        {
            found = false;
            for (j = 0; j < MAX_CUST_ATTACH; j++)
            {
                if (cust_socket->customizations[j] == NULL)
                {
                    // if NULL pointer found, then don't check remaining NULL pointers
                    break;
                }

                if (cust_nodes[i] == cust_socket->customizations[j])
                {
                    found = true;
                    break;
                }
            }
            if (found == false)
            {
                // found a new cust module to add to the socket
                assign_counter = assign_customization_to_array(cust_socket, cust_nodes[i]);
                if (assign_counter == 0 || assign_counter == MAX_CUST_ATTACH)
                {
                    // either assigning new customization failed or array is full, so stop trying
                    break;
                }
            }
        }
    }
}




// Hash for each possible b/c we know hash key and can limit search
struct customization_socket *get_cust_socket(struct task_struct *task, struct sock *sk)
{
    struct customization_socket *cust_socket = NULL;
    struct customization_socket *cust_socket_iterator;
    unsigned long key = task->pid ^ (unsigned long)sk;

    // expedite normal over cust
    spin_lock(&normal_socket_lock);
    hash_for_each_possible(normal_socket_table, cust_socket_iterator, socket_hash, key)
    {
        if (cust_socket_iterator->pid == task->pid && cust_socket_iterator->sk == sk)
        {
            cust_socket = cust_socket_iterator;
#ifdef DEBUG3
            trace_printk("L4.5: cust socket found in normal table, current pid %d, cust_pid %d\n", task->pid,
                         cust_socket_iterator->pid);
#endif
            break;
        }
    }
    spin_unlock(&normal_socket_lock);

    if (cust_socket == NULL)
    {
        spin_lock(&cust_socket_lock);
        hash_for_each_possible(cust_socket_table, cust_socket_iterator, socket_hash, key)
        {
            if (cust_socket_iterator->pid == task->pid && cust_socket_iterator->sk == sk)
            {
                cust_socket = cust_socket_iterator;
#ifdef DEBUG3
                trace_printk("L4.5: cust socket found in customization table, current pid %d, cust_pid %d\n", task->pid,
                             cust_socket_iterator->pid);
#endif
                break;
            }
        }
        spin_unlock(&cust_socket_lock);
    }
    return cust_socket;
}




// Called when new cust module registered
void set_update_cust_check(void)
{
    int bucket;
    struct customization_socket *cust_socket;
    struct hlist_node tmp;
    struct hlist_node *tmpptr = &tmp;
    int i;

    // all sockets in normal table are set
    spin_lock(&normal_socket_lock);
    hash_for_each_safe(normal_socket_table, bucket, tmpptr, cust_socket, socket_hash)
    {
#ifdef DEBUG
        trace_printk("L4.5 Normal Socket: Resetting things with pid %d and socket %p\n", cust_socket->pid,
                     cust_socket->sk);
#endif
        cust_socket->update_cust_check = true;
    }
    spin_unlock(&normal_socket_lock);

    // only sockets with space in cust array are set
    spin_lock(&cust_socket_lock);
    hash_for_each_safe(cust_socket_table, bucket, tmpptr, cust_socket, socket_hash)
    {
#ifdef DEBUG
        trace_printk("L4.5 Cust Socket: Resetting things with pid %d and socket %p\n", cust_socket->pid,
                     cust_socket->sk);
#endif
        for (i = 0; i < MAX_CUST_ATTACH; i++)
        {
            if (cust_socket->customizations[i] == NULL)
            {
                // there is room in the array for a new module to be added
                cust_socket->update_cust_check = true;
                break;
            }
        }
    }
    spin_unlock(&cust_socket_lock);

    return;
}




// Hash for each b/c we need to find matching sockets, so don't know keys
void remove_customization_from_each_socket(struct customization_node *cust)
{
    u32 cust_id = cust->cust_id;
    struct customization_socket *cust_socket_iterator;
    struct customization_node *cust_node;
    int bucket;
    struct hlist_node tmp;
    struct hlist_node *tmpptr = &tmp;
    size_t i;

    // Prevent customized socket access while potentially removing cust from it
    spin_lock(&cust_socket_lock);
    hash_for_each_safe(cust_socket_table, bucket, tmpptr, cust_socket_iterator, socket_hash)
    {
        for (i = 0; i < MAX_CUST_ATTACH; i++)
        {
            cust_node = cust_socket_iterator->customizations[i];
            if (cust_node == NULL)
            {
                // no need to keek checking
                break;
            }

            if (cust_node->cust_id == cust_id)
            {
#ifdef DEBUG1
                trace_printk("L4.5: Found socket mathching cust id=%d\n", cust_id);
#endif
                spin_lock(&cust_socket_iterator->active_customization_lock);
                // unassign_single_customization will also set update_cust_check to force another cust lookup
                unassign_single_customization(cust_socket_iterator, cust_node);
                spin_unlock(&cust_socket_iterator->active_customization_lock);

                // Remove socket from customization list
                hash_del(&cust_socket_iterator->socket_hash);

                // Now add to the normal list
                spin_lock(&normal_socket_lock);

                hash_add(normal_socket_table, &cust_socket_iterator->socket_hash, cust_socket_iterator->hash_key);
                spin_unlock(&normal_socket_lock);
                break;
            }
        }
    }
    spin_unlock(&cust_socket_lock);
    return;
}




// Only called by Socket close function
int delete_cust_socket(struct task_struct *task, struct sock *sk)
{
    int found = 0;
    struct customization_socket *cust_socket;
    cust_socket = get_cust_socket(task, sk);
    if (cust_socket != NULL)
    {
        found = 1;
        if (cust_socket->customizations[0] == NULL)
        {
            spin_lock(&normal_socket_lock);
            hash_del(&cust_socket->socket_hash);
            kfree(cust_socket);
            socket_allocsminusfrees--;
            spin_unlock(&normal_socket_lock);
        }
        else
        {
            spin_lock(&cust_socket_lock);
            hash_del(&cust_socket->socket_hash);
            // if socket close called on a customized socket then we allocated
            // buffer space for it
            unassign_all_customization(cust_socket);
            kfree(cust_socket);
            socket_allocsminusfrees--;
            spin_unlock(&cust_socket_lock);
        }
    }
    return found;
}




// Called when unloading DCA, so no customization modules can be loaded
void delete_all_cust_socket(void)
{
    int bucket;
    struct customization_socket *cust_socket;
    struct hlist_node tmp;
    struct hlist_node *tmpptr = &tmp;


    spin_lock(&normal_socket_lock);
    hash_for_each_safe(normal_socket_table, bucket, tmpptr, cust_socket, socket_hash)
    {
#ifdef DEBUG
        trace_printk("L4.5 Normal Socket: Deleting things in bucket [%d] with pid value %d and socket value %p\n",
                     bucket, cust_socket->pid, cust_socket->sk);
#endif
        hash_del(&cust_socket->socket_hash);
        kfree(cust_socket);
        socket_allocsminusfrees--;
    }
    spin_unlock(&normal_socket_lock);


    // nothing should be in this table, but check anyway to be safe
    spin_lock(&cust_socket_lock);
    hash_for_each_safe(cust_socket_table, bucket, tmpptr, cust_socket, socket_hash)
    {
#ifdef DEBUG
        trace_printk("L4.5 ALERT Cust Socket: Deleting things in bucket [%d] with pid value %d and socket value %p\n",
                     bucket, cust_socket->pid, cust_socket->sk);
#endif
        hash_del(&cust_socket->socket_hash);
        kfree(cust_socket);
        socket_allocsminusfrees--;
    }
    spin_unlock(&cust_socket_lock);
#ifdef DEBUG
    trace_printk("Socket kallocs minus kfrees: %i\n", socket_allocsminusfrees);
#endif
    return;
}




// If TCP socket, then msg pointer might be NULL because it does not have a msg yet
// NOTE: recv 4 tuple flips tuple to reflect a reply tuple
static void set_four_tuple(struct sock *sk, struct customization_socket *cust_socket, struct msghdr *msg)
{
    struct inet_sock *inet = inet_sk(sk);
    if (msg != NULL)
    {
        DECLARE_SOCKADDR(struct sockaddr_in *, sin, msg->msg_name); // for UDP

        if (sin)
        {
            cust_socket->socket_flow.dest_port = ntohs(sin->sin_port);
            cust_socket->socket_flow.dest_ip = sin->sin_addr.s_addr;
        }
    }
    else
    {
        cust_socket->socket_flow.dest_port = ntohs(sk->sk_dport);
        cust_socket->socket_flow.dest_ip = sk->sk_daddr;
    }

    cust_socket->socket_flow.source_port = ntohs(inet->inet_sport);
    cust_socket->socket_flow.source_ip = inet->inet_saddr;

    return;
}




// Assigns array of customizations to the socket; allocs buffers and sets params
// @param[X] cust_sock The customization socket to assign cust to
// @param[I] cust_node[MAX_CUST_ATTACH] The customization array to apply to the socket
// @param[I] node_counter The number of valid customiztion node pointers in the cust_node array
// @post customization buffers are allocated and customization params set
static void assign_customizations(struct customization_socket *cust_sock,
                                  struct customization_node *cust_node[MAX_CUST_ATTACH], size_t node_counter)
{
    u32 send_buffer_size = SEND_BUF_SIZE;
    u32 recv_buffer_size = RECV_BUF_SIZE;
    size_t i;

    // assume we will cust send/recv
    cust_sock->customize_or_skip = CUSTOMIZE;

    trace_printk("L4.5: assigning %lu nodes to socket\n", node_counter);

    // determine which customization node requires the largest buffer, and assign
    for (i = 0; i < node_counter; i++)
    {
        if (cust_node[i] == NULL)
        {
            break;
        }
        if (cust_node[i]->send_buffer_size > send_buffer_size)
        {
            send_buffer_size = cust_node[i]->send_buffer_size;
        }

        if (cust_node[i]->recv_buffer_size > recv_buffer_size)
        {
            recv_buffer_size = cust_node[i]->recv_buffer_size;
        }

        // all cust nodes must have valid function pointers, which are checked when module is registered
        if (cust_node[i]->send_function == NULL || cust_node[i]->recv_function == NULL)
        {
#ifdef DEBUG
            trace_printk("L4.5: Cust send or recv null, pid=%d, name=%s\n", cust_sock->pid,
                         cust_sock->socket_flow.task_name_pid);
#endif
            cust_sock->customize_or_skip = SKIP;
            break;
        }
    }


#ifdef DEBUG
    trace_printk("L4.5: Send buffer size = %u\n", send_buffer_size);
    trace_printk("L4.5: Recv buffer size = %u\n", recv_buffer_size);
#endif


    if (cust_sock->customize_or_skip == CUSTOMIZE)
    {
        cust_sock->send_buf_st.buf = kmalloc(send_buffer_size, GFP_KERNEL | __GFP_NOFAIL);
        cust_sock->recv_buf_st.buf = kmalloc(recv_buffer_size, GFP_KERNEL | __GFP_NOFAIL);
        if (cust_sock->send_buf_st.buf == NULL || cust_sock->recv_buf_st.buf == NULL)
        {
#ifdef DEBUG
            trace_printk("L4.5 ALERT cust buffer malloc fail; proto=%u, task=%s\n", cust_sock->socket_flow.protocol,
                         cust_sock->socket_flow.task_name_pid);
#endif
            cust_sock->customize_or_skip = SKIP;
        }
        else
        {
#ifdef DEBUG
            trace_printk("L4.5 Assigned buffer, pid=%d, name=%s\n", cust_sock->pid,
                         cust_sock->socket_flow.task_name_pid);
#endif

            socket_allocsminusfrees += 2;
            cust_sock->send_buf_st.buf_size = send_buffer_size;
            cust_sock->recv_buf_st.buf_size = recv_buffer_size;
            cust_sock->customize_or_skip = CUSTOMIZE;

            // only need to create the active spinlock if customizing socket
            spin_lock_init(&cust_sock->active_customization_lock);

            // need to loop through cust nodes and assign to socket
            for (i = 0; i < node_counter; i++)
            {
                cust_sock->customizations[i] = cust_node[i];
                cust_node[i]->sock_count += 1;
#ifdef DEBUG
                trace_printk("L4.5: Socket count = %u\n", cust_node[i]->sock_count);
#endif
            }
        }
    }

    return;
}




// Assigns a single customization to the socket cust array; reallocs buffers if necessary and sets params
// @param[X] cust_sock The customization socket to assign cust to
// @param[I] cust_node The customization node to add to the socket array
// @post customization buffers are allocated and customization params set
static size_t assign_customization_to_array(struct customization_socket *cust_sock,
                                            struct customization_node *cust_node)
{
    u32 send_buffer_size = cust_sock->send_buf_st.buf_size;
    u32 recv_buffer_size = cust_sock->recv_buf_st.buf_size;
    size_t i;
    void *temp_buffer;

    // need to get the lock because we are potentially adjusting buffers and order
    spin_lock(&cust_sock->active_customization_lock);

    // determine if we need a larger send buffer than previously allocated
    if (cust_node->send_buffer_size > cust_sock->send_buf_st.buf_size)
    {
        temp_buffer = krealloc(cust_sock->send_buf_st.buf, cust_node->send_buffer_size, GFP_KERNEL | __GFP_NOFAIL);

        if (temp_buffer == NULL)
        {
#ifdef DEBUG
            trace_printk("L4.5 ALERT cust buffer krealloc fail; proto=%u, task=%s\n", cust_sock->socket_flow.protocol,
                         cust_sock->socket_flow.task_name_pid);
#endif
            spin_unlock(&cust_sock->active_customization_lock);
            return 0;
        }
        cust_sock->send_buf_st.buf = temp_buffer;
        cust_sock->send_buf_st.buf_size = cust_node->send_buffer_size;
    }

    // determine if we need a larger send buffer than previously allocated
    if (cust_node->recv_buffer_size > cust_sock->recv_buf_st.buf_size)
    {
        temp_buffer = krealloc(cust_sock->recv_buf_st.buf, cust_node->recv_buffer_size, GFP_KERNEL | __GFP_NOFAIL);

        if (temp_buffer == NULL)
        {
#ifdef DEBUG
            trace_printk("L4.5 ALERT cust buffer krealloc fail; proto=%u, task=%s\n", cust_sock->socket_flow.protocol,
                         cust_sock->socket_flow.task_name_pid);
#endif
            // send buffer may be larger than required, but that is ok
            spin_unlock(&cust_sock->active_customization_lock);
            return 0;
        }
        cust_sock->recv_buf_st.buf = temp_buffer;
        cust_sock->recv_buf_st.buf_size = cust_node->recv_buffer_size;
    }

#ifdef DEBUG
    trace_printk("L4.5: Send buffer size = %u\n", send_buffer_size);
    trace_printk("L4.5: Recv buffer size = %u\n", recv_buffer_size);
#endif

    // now that buffer sizes are correct, add the cust node to socket and sort it


    // need to loop through cust nodes and assign to socket
    for (i = 0; i < MAX_CUST_ATTACH; i++)
    {
        if (cust_sock->customizations[i] == NULL)
        {
            cust_sock->customizations[i] = cust_node;
            cust_node->sock_count += 1;
#ifdef DEBUG
            trace_printk("L4.5: Socket count = %u\n", cust_node->sock_count);
#endif
            break;
        }
    }

    // now array may no longer be sorted, so sort again
    // for loop index + 1 = number of elements of customization array
    sort(cust_sock->customizations, i + 1, sizeof(void *), &priority_compare, NULL);

    spin_unlock(&cust_sock->active_customization_lock);

    return i + 1;
}




// Removes all customizations from the socket; resets values and frees buffers used
// @param[I] cust_sock The customization socket to remove any cust from
static void unassign_all_customization(struct customization_socket *cust_sock)
{
    struct customization_node *cust_node;
    size_t i;

    if (cust_sock->send_buf_st.buf != NULL)
    {
        kfree(cust_sock->send_buf_st.buf);
        cust_sock->send_buf_st.buf = NULL;
#ifdef DEBUG
        trace_printk("L4.5: Freed send buff, pid=%d, name=%s\n", cust_sock->pid, cust_sock->socket_flow.task_name_pid);
#endif
        socket_allocsminusfrees--;
    }

    if (cust_sock->recv_buf_st.buf != NULL)
    {
        kfree(cust_sock->recv_buf_st.buf);
        cust_sock->recv_buf_st.buf = NULL;
#ifdef DEBUG
        trace_printk("L4.5: Freed recv buff, pid=%d, name=%s\n", cust_sock->pid, cust_sock->socket_flow.task_name_pid);
#endif
        socket_allocsminusfrees--;
    }

    for (i = 0; i < MAX_CUST_ATTACH; i++)
    {
        // adjust the cust node sock count if necessary
        if (cust_sock->customizations[i] != NULL)
        {
            cust_node = cust_sock->customizations[i];
            cust_node->sock_count -= 1;
            cust_sock->customizations[i] = NULL;
        }
        else
        {
            break;
        }
    }

#ifdef DEBUG
    trace_printk("L4.5: Cust removed, pid=%d, name=%s\n", cust_sock->pid, cust_sock->socket_flow.task_name_pid);
#endif

    return;
}




// Removes a single customization from the socket; resets values and frees buffers used if necessary
// @param[I] cust_sock The customization socket to remove the cust from
static void unassign_single_customization(struct customization_socket *cust_sock, struct customization_node *cust_node)
{
    struct customization_node *temp_node[MAX_CUST_ATTACH] = {NULL};
    size_t i;
    size_t j;

    // if only one customization node attached, then call unassign all instead but with active_lock
    for (i = 0; i < MAX_CUST_ATTACH; i++)
    {
        if (cust_sock->customizations[i] == NULL)
        {
            break;
        }
        temp_node[i] = cust_sock->customizations[i];
    }

    spin_lock(&cust_sock->active_customization_lock);
    if (i == 0)
    {
        unassign_all_customization(cust_sock);
    }
    else
    {
        // remove the cust from the array
        for (i = 0; i < MAX_CUST_ATTACH; i++)
        {
            if (cust_sock->customizations[i] == NULL)
            {
                // found a NULL pointer, so no more cust to check
                break;
            }

            // find the matching sock to remove
            if (cust_sock->customizations[i] == cust_node)
            {
                cust_sock->customizations[i] = NULL;
                cust_node->sock_count -= 1;
                break;
            }
        }

        // then adjust array to fill in spot
        for (j = i; j + 1 < MAX_CUST_ATTACH; j++)
        {
            if (temp_node[j + 1] == NULL)
            {
                // no need to replace NULL with NULL
                break;
            }

            cust_sock->customizations[j] = temp_node[j + 1];
        }
    }
    spin_unlock(&cust_sock->active_customization_lock);

#ifdef DEBUG
    trace_printk("L4.5: Cust removed, pid=%d, name=%s\n", cust_sock->pid, cust_sock->socket_flow.task_name_pid);
#endif

    cust_sock->update_cust_check = true;


    return;
}




// Debug use only
#ifdef DEBUG
void print_all_cust_socket(void)
{
    int bucket;
    struct customization_socket *cust_socket;
    hash_for_each(cust_socket_table, bucket, cust_socket, socket_hash)
    {
        trace_printk("Bucket [%d] has pid=%d, name=%s\n", bucket, cust_socket->pid,
                     cust_socket->socket_flow.task_name_pid);
    }
    return;
}
#endif
