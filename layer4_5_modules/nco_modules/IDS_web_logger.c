// @file IDS_web_logger.c
// @brief An IDS customization module to modify python3 send/recv calls to detect a specific string
//

#include <linux/inet.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uio.h> // For iter structures
// ************** STANDARD PARAMS MUST GO HERE ****************
#include <helpers.h>
//  ************** END STANDARD PARAMS ****************
#include "../common_structs.h"
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

// Function to customize the msg sent from the application to layer 4
// @param[I] send_buf_st Pointer to the send buffer structure
// @param[I] socket_flow Pointer to the flow struct matching cust parameters
// @pre send_buf_st->src_iter holds app message destined for Layer 4
// @post send_buf_st->src_buf holds customized message for DCA to send to Layer 4
void modify_buffer_send(struct customization_buffer *send_buf_st, struct customization_flow *socket_flow)
{
    // IDS variables
    char *search_str = "exampleurl.com"; // The string to search for
    char *found_str = NULL;
    char alert_str[8] = "BLOCKED"; // char array for alert message
    bool copy_success;

    send_buf_st->copy_length = 0;
    set_module_struct_flags(send_buf_st, false);

    // if module hasn't been activated, then don't perform customization
    if (*python_cust->active_mode == 0)
    {
        send_buf_st->try_next = true;
        return;
    }

    // copy data from src_iter to buf
    copy_success = copy_from_iter_full(send_buf_st->buf, send_buf_st->length, send_buf_st->src_iter);
    if (copy_success == false)
    {
        // not all bytes were copied, so pick scenario 1 or 2 below
        trace_printk("L4.5 ALERT: Failed to copy all bytes to cust buffer\n");
        // Scenario 1: keep cust loaded and allow normal msg to be sent
        send_buf_st->copy_length = 0;
    }

    // Ensure the buffer is null-terminated before searching (this prevents long strings from generating ALERT flag)
    if (send_buf_st->length < send_buf_st->buf_size)
    {
        ((char *)send_buf_st->buf)[send_buf_st->length] = '\0';
    }

    // search for substring within buffer
    found_str = strstr(send_buf_st->buf, search_str);

    // clear IDS string to prevent duplicate ALERT messages
    strscpy(IDS, "", sizeof(IDS));

    if (found_str != NULL)
    {
        // if search string is found, set IDS to alert message
        strscpy(IDS, "ALERT:STRING1", sizeof(IDS));

        // Ensure enough space in buffer before modifying it
        size_t alert_len = sizeof(alert_str) - 1; // Exclude null terminator
        size_t total_needed = send_buf_st->length + alert_len;

        if (total_needed <= send_buf_st->buf_size)
        {
            // Move existing buffer contents right to make space for alert_str
            memmove(send_buf_st->buf + alert_len, send_buf_st->buf, send_buf_st->length);

            // Insert modification to prevent connection to malicious URL
            memcpy(send_buf_st->buf, alert_str, alert_len);

            // Update copy_length
            send_buf_st->copy_length = total_needed;
        }
        else
        {
            trace_printk("L4.5 ALERT: Not enough space to prepend alert message\n");
            send_buf_st->copy_length = send_buf_st->length; // Just send original message
        }

        // Debug output
        trace_printk("L4.5: send buffer contents if ALERT: %s\n", send_buf_st->buf);
    }
    else
    {

        // traceprintK for debugging
        trace_printk("L4.5: send buffer contents: %s\n", send_buf_st->buf);
        send_buf_st->copy_length = send_buf_st->length;
    }

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
    char thread_name[16] = "*";
    char application_name[16] = "*";
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

    python_cust->target_flow.protocol = 6; // TCP
    // python_cust->protocol = 6; // TCP
    // python_cust->target_flow.protocol = (u16)protocol; // TCP and UDP
    memcpy(python_cust->target_flow.task_name_pid, thread_name, TASK_NAME_LEN);
    memcpy(python_cust->target_flow.task_name_tgid, application_name, TASK_NAME_LEN);

    // Client: no source IP or port set unless client called bind first
    // python_cust->target_flow.dest_port = (u16)destination_port;
    // python_cust->target_flow.source_port = (u16)source_port;
    python_cust->target_flow.dest_port = 8080; // HTTP requests
    python_cust->target_flow.source_port = 0;  // any source port

    // IP is a __be32 value
    // python_cust->target_flow.dest_ip = in_aton(destination_ip);
    // python_cust->target_flow.source_ip = in_aton(source_ip);
    // python_cust->target_flow.dest_ip = in_aton("127.0.0.1");
    python_cust->target_flow.dest_ip = 0;
    python_cust->target_flow.source_ip = 0;

    // These functions must be defined and will be used to modify the
    // buffer on a send/receive call
    python_cust->send_function = modify_buffer_send;
    python_cust->recv_function = modify_buffer_recv;

    python_cust->send_buffer_size = 2048; // we don't need a full buffer
    python_cust->recv_buffer_size = 32;   // we don't plan to use this buffer

    // Cust ID normally set by NCO, uniqueness required
    python_cust->cust_id = module_id;
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
