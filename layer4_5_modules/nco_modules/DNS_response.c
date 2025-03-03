// @file DNS_response.c
// @brief An IDS customization module to monitor DNS queries and generate an alert if rate is exceeded

#include <linux/inet.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uio.h> // For iter structures
// ************** STANDARD PARAMS MUST GO HERE ****************
#include <helpers.h>
#include "../common_structs.h"
//  ************** END STANDARD PARAMS ****************

extern int register_customization(struct customization_node *cust, u16 applyNow);

extern int unregister_customization(struct customization_node *cust);

extern void trace_print_hex_dump(const char *prefix_str, int prefix_type, int rowsize, int groupsize, const void *buf,
                                 size_t len, bool ascii);

extern void set_module_struct_flags(struct customization_buffer *buf, bool flag_set);

struct customization_node *python_cust;
char IDS[16];

// NCO VARIABLES GO HERE
u16 module_id = 1;
char hex_key[HEX_KEY_LENGTH] = "";
u16 activate = 0;
u16 priority = 0;
u16 applyNow = 0;
// END NCO VARIABLES

void monitor_dns_queries(struct customization_flow *socket_flow)
{
    static unsigned int dns_query_count = 0;
    static unsigned long last_reset_time = 0;
    unsigned long current_time = jiffies;

    if (time_before_eq(last_reset_time + 10 * HZ, current_time))
    {
        dns_query_count = 0;
        last_reset_time = current_time;

        // Reset IDS when the rate drops below the threshold
        strscpy(IDS, "", sizeof(IDS));
        trace_printk("L4.5: DNS query rate normalized. IDS reset.\n");
    }

    dns_query_count++;
    trace_printk("L4.5: DNS Query Count: %u\n", dns_query_count);

    if (dns_query_count > 5)
    {
        strscpy(IDS, "ALERT:DNS", sizeof(IDS));
        trace_printk("L4.5: DNS query rate exceeded. Content of IDS: %s\n", IDS);
    }
}

// Function to customize the msg sent from the application to layer 4
// @param[I] send_buf_st Pointer to the send buffer structure
// @param[I] socket_flow Pointer to the flow struct matching cust parameters
// @pre send_buf_st->src_iter holds app message destined for Layer 4
// @post send_buf_st->src_buf holds customized message for DCA to send to Layer 4
void modify_buffer_send(struct customization_buffer *send_buf_st, struct customization_flow *socket_flow)
{
    // Only proceed if the process name contains "isc-socket"
    /*if (strstr(socket_flow->task_name_tgid, "isc-socket") == NULL)
    {
        trace_printk("L4.5: Ignoring process %s, only monitoring isc-socket threads\n", socket_flow->task_name_tgid);
        send_buf_st->try_next = true; // Allow normal processing for other processes
        return;
    }*/
    monitor_dns_queries(socket_flow);
    send_buf_st->copy_length = 0;

    set_module_struct_flags(send_buf_st, false);

    // if module hasn't been activated, then don't perform customization
    if (*python_cust->active_mode == 0)
    {
        send_buf_st->try_next = true;
        return;
    }

    // not customizing send path
    send_buf_st->no_cust = true;
    return;
}

// Function to customize the msg received from L4 prior to delivery to application
// @param[I] recv_buf_st Pointer to the recv buffer structure
// @param[I] socket_flow Pointer to the flow struct mathing cust parameters
// @pre recv_buf_st->src_iter holds app message destined for application
// @post recv_buf_st->buf holds customized message for DCA to send to app instead
// NOTE: copy_length must be <= length
void modify_buffer_recv(struct customization_buffer *recv_buf_st, struct customization_flow *socket_flow)
{
    monitor_dns_queries(socket_flow);
    set_module_struct_flags(recv_buf_st, false);

    // if module hasn't been activated, then don't perform customization
    if (*python_cust->active_mode == 0)
    {
        recv_buf_st->try_next = true;
        return;
    }

    // no cust being performed on recv path
    recv_buf_st->no_cust = true;
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
    char thread_name[16] = "*";      // changed from "python3"
    char application_name[16] = "*"; // changed from "python3"
    int result;

    python_cust = kmalloc(sizeof(struct customization_node), GFP_KERNEL);
    if (python_cust == NULL)
    {
        trace_printk("L4.5 ALERT: client kmalloc failed\n");
        return -1;
    }

    // IDS additions
    python_cust->IDS_ptr = IDS;
    python_cust->security_mod = true;

    // provide pointer for DCA to toggle active mode instead of new function
    python_cust->active_mode = &activate;

    // provide pointer for DCA to update priority instead of new function
    python_cust->cust_priority = &priority;

    // python_cust->target_flow.protocol = 17; // UDP
    //  python_cust->protocol = 6; // TCP
    python_cust->target_flow.protocol = 256; // match any layer 4 protocol
    memcpy(python_cust->target_flow.task_name_pid, thread_name, TASK_NAME_LEN);
    memcpy(python_cust->target_flow.task_name_tgid, application_name, TASK_NAME_LEN);

    // Client: no source IP or port set unless client called bind first
    python_cust->target_flow.dest_port = 53;
    python_cust->target_flow.source_port = 0;

    // IP is a __be32 value
    python_cust->target_flow.dest_ip = "0.0.0.0";
    python_cust->target_flow.source_ip = "0.0.0.0";

    // These functions must be defined and will be used to modify the
    // buffer on a send/receive call
    python_cust->send_function = modify_buffer_send;
    python_cust->recv_function = modify_buffer_recv;

    python_cust->send_buffer_size = 2048; // we don't need a full buffer
    python_cust->recv_buffer_size = 32;   // we don't plan to use this buffer

    // Cust ID normally set by NCO, uniqueness required
    python_cust->cust_id = 53; // because DNS
    python_cust->registration_time_struct.tv_sec = 0;
    python_cust->registration_time_struct.tv_nsec = 0;
    python_cust->revoked_time_struct.tv_sec = 0;
    python_cust->revoked_time_struct.tv_nsec = 0;

    result = register_customization(python_cust, applyNow);
    if (result != 0)
    {
        trace_printk("L4.5 ALERT: Module failed registration, check debug logs\n");
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
        trace_printk("L4.5 ALERT: client module unload error\n");
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
