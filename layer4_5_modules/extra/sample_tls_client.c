// @file sample_python_client.c
// @brief A sample customization module to modify python3 send/recv calls

#include <linux/inet.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uio.h> // For iter structures

// ************** STANDARD PARAMS MUST GO HERE ****************
#include <common_structs.h>
#include <printing.h>
// ************** END STANDARD PARAMS ****************


extern int register_customization(struct customization_node *cust, bool applyNow);

extern int unregister_customization(struct customization_node *cust);

// Kernel module parameters with default values
static char *destination_ip = "10.0.0.20";
module_param(destination_ip, charp, 0600); // root only access to change
MODULE_PARM_DESC(destination_ip, "Dest IP to match");

static char *source_ip = "0.0.0.0";
module_param(source_ip, charp, 0600);
MODULE_PARM_DESC(source_ip, "Dest IP to match");

static unsigned int destination_port = 443;
module_param(destination_port, uint, 0600);
MODULE_PARM_DESC(destination_port, "DPORT to match");

static unsigned int source_port = 0;
module_param(source_port, uint, 0600);
MODULE_PARM_DESC(source_port, "SPORT to match");

static unsigned int protocol = 6; //  UDP
module_param(protocol, uint, 0600);
MODULE_PARM_DESC(protocol, "L4 protocol to match");

static bool applyNow = false;
module_param(applyNow, bool, 0600);
MODULE_PARM_DESC(protocol, "Apply customization lookup to all sockets, not just new sockets");

unsigned short bypass = 0;
module_param(bypass, ushort, 0600);
MODULE_PARM_DESC(bypass, "Place customization in bypass mode, which bypasses customization");

unsigned short priority = 65535;
module_param(priority, ushort, 0600);
MODULE_PARM_DESC(priority, "Customization priority level used when attaching modules to socket");


// test message for this module
char cust_test[12] = "testCustMod";

struct customization_node *curl_cust;



// this just does a straight copy operation
void modify_buffer_send(struct customization_buffer *send_buf_st, struct customization_flow *socket_flow)
{
    bool copy_success;
    size_t cust_test_size = (size_t)sizeof(cust_test) - 1;

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
    size_t cust_test_size = (size_t)sizeof(cust_test) - 1;

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
// Client will check parameters on first sendmsg
// 0 used as default to skip port or IP checks
// protocol = 256 -> match any layer 4 protocol
// * used as a wildcard for application name to match all
// @post Layer 4.5 customization registered
int __init sample_client_start(void)
{
    char thread_name[16] = "*";
    char application_name[16] = "*";
    int result;

    curl_cust = kmalloc(sizeof(struct customization_node), GFP_KERNEL);
    if (curl_cust == NULL)
    {
        trace_printk("L4.5 ALERT: client kmalloc failed\n");
        return -1;
    }

    // provide pointer for DCA to toggle bypass mode instead of new function
    curl_cust->bypass_mode = &bypass;

    // provide pointer for DCA to update priority instead of new function
    curl_cust->cust_priority = &priority;

    curl_cust->target_flow.protocol = (u16)protocol;
    memcpy(curl_cust->target_flow.task_name_pid, thread_name, TASK_NAME_LEN);
    memcpy(curl_cust->target_flow.task_name_tgid, application_name, TASK_NAME_LEN);

    // Client: no source IP or port set unless client called bind first
    curl_cust->target_flow.dest_port = (u16)destination_port;
    curl_cust->target_flow.source_port = (u16)source_port;

    // IP is a __be32 value
    curl_cust->target_flow.dest_ip = in_aton(destination_ip);
    curl_cust->target_flow.source_ip = in_aton(source_ip);

    // These functions must be defined and will be used to modify the
    // buffer on a send/receive call
    // Function can be set to NULL if not modifying buffer contents
    curl_cust->send_function = modify_buffer_send;
    curl_cust->recv_function = modify_buffer_recv;

    // Cust ID normally set by NCO, uniqueness required
    curl_cust->cust_id = 42;
    curl_cust->registration_time_struct.tv_sec = 0;
    curl_cust->registration_time_struct.tv_nsec = 0;
    curl_cust->revoked_time_struct.tv_sec = 0;
    curl_cust->revoked_time_struct.tv_nsec = 0;

    result = register_customization(curl_cust, applyNow);

    if (result != 0)
    {
        trace_printk("L4.5 ALERT: Module failed registration, check debug logs\n");
        return -1;
    }

    trace_printk("L4.5: client module loaded, id=%d\n", curl_cust->cust_id);

    return 0;
}


// Calls the functions to unregister customization node from use on sockets
// @post Layer 4.5 customization node unregistered
void __exit sample_client_end(void)
{
    // NOTE: this is only valid/safe place to call unregister (deadlock scenario)
    int ret = unregister_customization(curl_cust);

    if (ret == 0)
    {
        trace_printk("L4.5 ALERT: client module unload error\n");
    }
    else
    {
        trace_printk("L4.5: client module unloaded\n");
    }
    kfree(curl_cust);
    return;
}


module_init(sample_client_start);
module_exit(sample_client_end);
MODULE_AUTHOR("Dan Lukaszewski");
MODULE_LICENSE("GPL");
