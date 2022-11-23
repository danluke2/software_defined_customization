// @file sample_python_server.c
// @brief A sample customization module to modify python3 send/recv calls

#include <linux/inet.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uio.h> // For iter structures

// ************** STANDARD PARAMS MUST GO HERE ****************
#include <common_structs.h>
#include <helpers.h>
// ************** END STANDARD PARAMS ****************


extern int register_customization(struct customization_node *cust, u16 applyNow);

extern int unregister_customization(struct customization_node *cust);

extern void trace_print_hex_dump(const char *prefix_str, int prefix_type, int rowsize, int groupsize, const void *buf,
                                 size_t len, bool ascii);

extern void set_module_struct_flags(struct customization_buffer *buf, bool flag_set);

// Kernel module parameters with default values
static char *destination_ip = "10.0.0.40";
module_param(destination_ip, charp, 0600); // root only access to change
MODULE_PARM_DESC(destination_ip, "Dest IP to match");

static char *source_ip = "10.0.0.20";
module_param(source_ip, charp, 0600);
MODULE_PARM_DESC(source_ip, "Dest IP to match");

static unsigned int destination_port = 0;
module_param(destination_port, uint, 0600);
MODULE_PARM_DESC(destination_port, "DPORT to match");

static unsigned int source_port = 443;
module_param(source_port, uint, 0600);
MODULE_PARM_DESC(source_port, "SPORT to match");

static unsigned int protocol = 6; //  UDP
module_param(protocol, uint, 0600);
MODULE_PARM_DESC(protocol, "L4 protocol to match");

static unsigned short applyNow = 0;
module_param(applyNow, ushort, 0600);
MODULE_PARM_DESC(protocol, "Apply customization lookup to all sockets, not just new sockets");

unsigned short activate = 1;
module_param(activate, ushort, 0600);
MODULE_PARM_DESC(activate, "Place customization in active mode, which enables customization");

unsigned short priority = 65535;
module_param(priority, ushort, 0600);
MODULE_PARM_DESC(priority, "Customization priority level used when attaching modules to socket");

// test message for this module
char cust_test[12] = "testCustMod";


struct customization_node *nginx_cust;


// this just does a straight copy operation
void modify_buffer_send(struct customization_buffer *send_buf_st, struct customization_flow *socket_flow)
{
    bool copy_success;
    //-1 b/c don't want terminating part
    size_t cust_test_size = (size_t)sizeof(cust_test) - 1;


    set_module_struct_flags(send_buf_st, false);

    // if module hasn't been activated, then don't perform customization
    if (*nginx_cust->active_mode == 0)
    {
        send_buf_st->try_next = true;
        return;
    }

    send_buf_st->copy_length = send_buf_st->length;
    trace_printk("L4.5: TLS send buffer size %lu\n", send_buf_st->length);

    // copy from full will revert iter back to normal if failure occurs
    copy_success = copy_from_iter_full(send_buf_st->buf, send_buf_st->length, send_buf_st->src_iter);
    if (copy_success == false)
    {
        // not all bytes were copied, so pick scenario 1 or 2 below
        trace_printk("L4.5 ALERT: Failed to copy all bytes to cust buffer\n");
        // Scenario 1: keep cust loaded and allow normal msg to be sent
        send_buf_st->copy_length = 0;

        // Scenario 2: stop trying to customize this socket
        // kfree(send_buf_st->buf);
        // send_buf_st->buf = NULL;
        // copy_length = 0;
    }
    return;
}


void modify_buffer_recv(struct customization_buffer *recv_buf_st, struct customization_flow *socket_flow)
{
    bool copy_success;
    //-1 b/c don't want terminating part
    size_t cust_test_size = (size_t)sizeof(cust_test) - 1;

    set_module_struct_flags(recv_buf_st, false);

    // if module hasn't been activated, then don't perform customization
    if (*nginx_cust->active_mode == 0)
    {
        recv_buf_st->try_next = true;
        return;
    }

    trace_printk("L4.5: TLS recv ret %lu, buffer limit %d\n", recv_buf_st->recv_return, recv_buf_st->length);

    recv_buf_st->copy_length = recv_buf_st->recv_return;

    copy_success = copy_from_iter_full(recv_buf_st->buf, recv_buf_st->copy_length, recv_buf_st->src_iter);
    if (copy_success == false)
    {
        // not all bytes were copied, so pick scenario 1 or 2 below
        trace_printk("L4.5 ALERT: Failed to copy all bytes to cust buffer\n");
        // Scenario 1: keep cust loaded and allow normal msg to be sent
        recv_buf_st->copy_length = 0;

        // Scenario 2: stop trying to customize this socket
        // kfree(recv_buf_st->buf);
        // recv_buf_st->buf = NULL;
        // copy_length = 0;
    }
    return;
}


// The init function that calls the functions to register a Layer 4.5 customization
// Server will check parameters on first recvmsg
// 0 used as default to skip port or IP checks
// protocol = 256 -> match any layer 4 protocol
// * used as a wildcard for application name to match all [TODO: test]
// @post Layer 4.5 customization registered
int __init sample_server_start(void)
{
    // exact application name (i.e., will use strcmp function)
    char thread_name[16] = "nginx";      // 16 b/c size of kernel char array
    char application_name[16] = "nginx"; // 16 b/c size of kernel char array
    int result;

    nginx_cust = kmalloc(sizeof(struct customization_node), GFP_KERNEL);
    if (nginx_cust == NULL)
    {
        trace_printk("L4.5 ALERT: server kmalloc failed\n");
        return -1;
    }

    // provide pointer for DCA to toggle active mode instead of new function
    nginx_cust->active_mode = &activate;

    // provide pointer for DCA to update priority instead of new function
    nginx_cust->cust_priority = &priority;

    nginx_cust->target_flow.protocol = (u16)protocol;
    memcpy(nginx_cust->target_flow.task_name_pid, thread_name, TASK_NAME_LEN);
    memcpy(nginx_cust->target_flow.task_name_tgid, application_name, TASK_NAME_LEN);

    // Server: source IP or port set b/c bind is called at setup
    nginx_cust->target_flow.dest_port = (u16)destination_port; // set if you know client port
    nginx_cust->target_flow.source_port = (u16)source_port;

    // IP is a __be32 value
    nginx_cust->target_flow.dest_ip = in_aton(destination_ip);
    nginx_cust->target_flow.source_ip = in_aton(source_ip);

    nginx_cust->send_function = modify_buffer_send;
    nginx_cust->recv_function = modify_buffer_recv;

    nginx_cust->send_buffer_size = 65536 * 2;
    nginx_cust->recv_buffer_size = 65536 * 2;

    // Cust ID normally set by NCO, uniqueness required
    nginx_cust->cust_id = 24;
    nginx_cust->registration_time_struct.tv_sec = 0;
    nginx_cust->registration_time_struct.tv_nsec = 0;
    nginx_cust->revoked_time_struct.tv_sec = 0;
    nginx_cust->revoked_time_struct.tv_nsec = 0;

    result = register_customization(nginx_cust, applyNow);

    if (result != 0)
    {
        trace_printk("L4.5 ALERT: Module failed registration, check debug logs\n");
        return -1;
    }

    trace_printk("L4.5: server module loaded, id=%d\n", nginx_cust->cust_id);

    return 0;
}

// Calls the functions to unregister customization node from use on sockets
// @post Layer 4.5 customization node unregistered
void __exit sample_server_end(void)
{
    int ret = unregister_customization(nginx_cust);

    if (ret == 0)
    {
        trace_printk("L4.5 ALERT: server module unload error\n");
    }
    else
    {
        trace_printk("L4.5: server module unloaded\n");
    }
    kfree(nginx_cust);
    return;
}

module_init(sample_server_start);
module_exit(sample_server_end);
MODULE_AUTHOR("Dan Lukaszewski");
MODULE_LICENSE("GPL");
