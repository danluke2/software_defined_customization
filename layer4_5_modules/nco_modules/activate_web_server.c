// @file test_modules/overhead_test_app_tag_server.c
// @brief The customization module to add app tag to test overhead

#include <linux/delay.h> // kernel sleep functions
#include <linux/inet.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uio.h> // For iter structures


// ************** STANDARD PARAMS MUST GO HERE ****************
#include "../common_structs.h"
// ************** END STANDARD PARAMS ****************


static int __init sample_client_start(void);
static void __exit sample_client_end(void);


extern int register_customization(struct customization_node *cust, u16 applyNow);

extern int unregister_customization(struct customization_node *cust);

extern void trace_print_hex_dump(const char *prefix_str, int prefix_type, int rowsize, int groupsize, const void *buf,
                                 size_t len, bool ascii);

extern void set_module_struct_flags(struct customization_buffer *buf, bool flag_set);


// Kernel module parameters with default values
static char *destination_ip = "0.0.0.0";
module_param(destination_ip, charp, 0600); // root only access to change
MODULE_PARM_DESC(destination_ip, "Dest IP to match");

static char *source_ip = "0.0.0.0";
module_param(source_ip, charp, 0600);
MODULE_PARM_DESC(source_ip, "Dest IP to match");

static unsigned int destination_port = 0;
module_param(destination_port, uint, 0600);
MODULE_PARM_DESC(destination_port, "DPORT to match");

static unsigned int source_port = 8080;
module_param(source_port, uint, 0600);
MODULE_PARM_DESC(source_port, "SPORT to match");

static unsigned int protocol = 6; // TCP or UDP
module_param(protocol, uint, 0600);
MODULE_PARM_DESC(protocol, "L4 protocol to match");


static unsigned int sleep_time = 100; // TCP or UDP
module_param(sleep_time, uint, 0600);
MODULE_PARM_DESC(sleep_time, "Number of msec to sleep for");

struct customization_node *server_cust;


// NCO VARIABLES GO HERE
u16 module_id = 1;
char hex_key[HEX_KEY_LENGTH] = "";
u16 activate = 0;
u16 priority = 0;
u16 applyNow = 0;


// END NCO VARIABLES


// Function to customize the msg sent from the application to layer 4
// @param[I] send_buf_st Pointer to the send buffer structure
// @param[I] socket_flow Pointer to the socket flow parameters
// @pre send_buf_st->src_iter holds app message destined for Layer 4
// @post src_buf holds customized message for PCM to send to Layer 4
void modify_buffer_send(struct customization_buffer *send_buf_st, struct customization_flow *socket_flow)
{
    send_buf_st->copy_length = 0;

    set_module_struct_flags(send_buf_st, false);

    // if module hasn't been activated, then don't perform customization
    if (*server_cust->active_mode == 0)
    {
        send_buf_st->try_next = true;
    }

    else
    {
        msleep(sleep_time);
        send_buf_st->no_cust = true;
    }

    return;
}


// Function to customize the msg recieved from L4 prior to delivery to application
void modify_buffer_recv(struct customization_buffer *recv_buf_st, struct customization_flow *socket_flow)
{
    set_module_struct_flags(recv_buf_st, false);

    // if module hasn't been activated, then don't perform customization
    if (*server_cust->active_mode == 0)
    {
        recv_buf_st->try_next = true;
        return;
    }


    recv_buf_st->no_cust = true;
    return;
}



// The init function that calls the functions to register a Layer 4.5 customization
// @post Layer 4.5 customization registered
int __init sample_client_start(void)
{
    char thread_name[16] = "python3";
    char application_name[16] = "python3";
    int result;

    server_cust = kmalloc(sizeof(struct customization_node), GFP_KERNEL);
    if (server_cust == NULL)
    {
        trace_printk("L4.5 ALERT: server kmalloc failed\n");
        return -1;
    }

    // provide pointer for DCA to toggle active mode instead of new function
    server_cust->active_mode = &activate;

    // provide pointer for DCA to update priority instead of new function
    server_cust->cust_priority = &priority;

    server_cust->target_flow.protocol = (u16)protocol; // TCP
    memcpy(server_cust->target_flow.task_name_pid, thread_name, TASK_NAME_LEN);
    memcpy(server_cust->target_flow.task_name_tgid, application_name, TASK_NAME_LEN);

    server_cust->target_flow.dest_port = (u16)destination_port;
    server_cust->target_flow.source_port = (u16)source_port;

    server_cust->target_flow.dest_ip = in_aton(destination_ip);
    server_cust->target_flow.source_ip = in_aton(source_ip);

    server_cust->send_function = modify_buffer_send;
    server_cust->recv_function = modify_buffer_recv;

    server_cust->cust_id = module_id;
    server_cust->registration_time_struct.tv_sec = 0;
    server_cust->registration_time_struct.tv_nsec = 0;
    server_cust->revoked_time_struct.tv_sec = 0;
    server_cust->revoked_time_struct.tv_nsec = 0;

    server_cust->send_buffer_size = 65536 * 2; // bufer
    server_cust->recv_buffer_size = 0;         // accept default buffer size

    result = register_customization(server_cust, applyNow);

    if (result != 0)
    {
        trace_printk("L4.5 ALERT: Module failed registration, check debug logs\n");
        return -1;
    }

    trace_printk("L4.5: server module loaded, id=%d\n", server_cust->cust_id);

    return 0;
}


// Calls the functions to unregister customization node from use on sockets
// @post Layer 4.5 customization node unregistered
void __exit sample_client_end(void)
{
    // NOTE: this is only valid/safe place to call unregister (deadlock scenario)
    int ret = unregister_customization(server_cust);

    if (ret == 0)
    {
        trace_printk("L4.5 ALERT: server module unload error\n");
    }
    else
    {
        trace_printk("L4.5: server module unloaded\n");
    }
    kfree(server_cust);
    return;
}



module_init(sample_client_start);
module_exit(sample_client_end);
MODULE_AUTHOR("Dan Lukaszewski");
MODULE_LICENSE("GPL");
