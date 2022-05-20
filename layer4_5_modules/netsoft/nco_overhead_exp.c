// @file core_modules/NCO_test.c
// @brief Test module to verify challenge-response works

#include <linux/inet.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uio.h> // For iter structures
#include <linux/version.h>

// for the overhead test, a copy of common_structs is in same location as this module
#include "common_structs.h"


extern int register_customization(struct customization_node *cust, bool applyNow);

extern int unregister_customization(struct customization_node *cust);

// test message for this plugin
char cust_test[12] = "testCustMod";

struct customization_node *python_cust;


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

static unsigned int source_port = 53;
module_param(source_port, uint, 0600);
MODULE_PARM_DESC(source_port, "SPORT to match");

static unsigned int protocol = 17; // UDP
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


// NCO VARIABLES GO HERE
u16 module_id = 1;
char hex_key[HEX_KEY_LENGTH] = "";
u16 bypass = 0;
u16 priority = 0;
u16 applyNow = 0;


// END NCO VARIABLES



// Function to customize the msg sent from the application to layer 4
// @param[I] send_buf_st Pointer to the send buffer structure
// @param[I] socket_flow Pointer to the socket flow parameters
// @param[I] send_buf_st->src_iter Application message structure to copy from
// @param[I] send_buf_st->length Total size of the application message
// @param[X] send_buf_st->copy_length Number of bytes DCA needs to send to layer 4
// @pre src_iter holds app message destined for Layer 4
// @post src_buf holds customized message for DCA to send to Layer 4
void modify_buffer_send(struct customization_buffer *send_buf_st, struct customization_flow *socket_flow)
{
    // passive monitoring allowed here, but active must pass bypass_mode check

    if (*python_cust->bypass_mode)
    {
        send_buf_st->try_next = true;
        return;
    }


    send_buf_st->no_cust = true;
    return;
}


// Function to customize the msg recieved from L4 prior to delivery to application
// @param[I] recv_buf_st Pointer to the recv buffer structure
// @param[I] socket_flow Pointer to the socket flow parameters
// @pre recv_buf_st->src_iter holds app message destined for application
// @post recv_buf holds customized message for DCA to send to app instead
void modify_buffer_recv(struct customization_buffer *recv_buf_st, struct customization_flow *socket_flow)
{
    // passive monitoring allowed here, but active must pass bypass_mode check

    if (*python_cust->bypass_mode)
    {
        recv_buf_st->try_next = true;
        return;
    }


    recv_buf_st->no_cust = true;
    return;
}



// The init function that calls the functions to register a Layer 4.5 customization
// Client will check parameters on first sendmsg
// 0 used as default to skip port or IP checks
// protocol = 256 -> match any layer 4 protocol
// * used as a wildcard for application name to match all [TODO: test]
// @post Layer 4.5 customization registered
int __init sample_client_start(void)
{
    char thread_name[16] = "python3";
    char application_name[16] = "python3";
    int result;


    python_cust = kmalloc(sizeof(struct customization_node), GFP_KERNEL);
    if (python_cust == NULL)
    {
        trace_printk("client kmalloc failed\n");
        return -1;
    }

    // provide pointer for DCA to toggle bypass mode instead of new function
    python_cust->bypass_mode = &bypass;

    // provide pointer for DCA to update priority instead of new function
    python_cust->cust_priority = &priority;

    python_cust->target_flow.protocol = 17; // UDP
                                            // python_cust->protocol = 6; // TCP
                                            // python_cust->protocol = 256; // Any since not valid IP field value
    memcpy(python_cust->target_flow.task_name_pid, thread_name, TASK_NAME_LEN);
    memcpy(python_cust->target_flow.task_name_tgid, application_name, TASK_NAME_LEN);

    // Client: no source IP or port set unless client called bind first
    python_cust->target_flow.dest_port = 65432;
    python_cust->target_flow.source_port = 0;

    // IP is a __be32 value
    python_cust->target_flow.dest_ip = in_aton("127.0.0.1");
    python_cust->target_flow.source_ip = 0;

    // These functions must be defined and will be used to modify the
    // buffer on a send/receive call
    // Function can be set to NULL if not modifying buffer contents
    python_cust->send_function = modify_buffer_send;
    python_cust->recv_function = modify_buffer_recv;

    // Cust ID set by customization controller, network uniqueness required
    python_cust->cust_id = module_id;
    python_cust->registration_time_struct.tv_sec = 0;
    python_cust->registration_time_struct.tv_nsec = 0;
    python_cust->revoked_time_struct.tv_sec = 0;
    python_cust->revoked_time_struct.tv_nsec = 0;

    result = register_customization(python_cust, applyNow);

    if (result != 0)
    {
        trace_printk("L4.5 AELRT: Module failed registration, check debug logs\n");
        return -1;
    }

    trace_printk("L4.5: client module loaded, id=%d\n", python_cust->cust_id);

    return 0;
}


// Calls the functions to unregister customization node from use on sockets
// @post Layer 4.5 customization node unregistered
void __exit sample_client_end(void)
{
    // NOTE: this is only valid/safe place to call unregister (deadlock scenario)
    int ret = unregister_customization(python_cust);

    if (ret == 0)
    {
        trace_printk("L4.5 AELRT: client module unload error\n");
    }
    else
    {
        trace_printk("L4.5: client module unloaded\n");
    }
    kfree(python_cust);
    return;
}



module_init(sample_client_start);
module_exit(sample_client_end);
MODULE_AUTHOR("Dan Lukaszewski");
MODULE_LICENSE("GPL");
