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
#include <helpers.h>
// ************** END STANDARD PARAMS ****************



extern int register_customization(struct customization_node *cust, u16 applyNow);

extern int unregister_customization(struct customization_node *cust);

extern void trace_print_hex_dump(const char *prefix_str, int prefix_type, int rowsize, int groupsize, const void *buf,
                                 size_t len, bool ascii);

extern void set_module_struct_flags(struct customization_buffer *buf, bool flag_set);

// Kernel module parameters with default values
static char *destination_ip = "127.0.0.1";
module_param(destination_ip, charp, 0600); // root only access to change
MODULE_PARM_DESC(destination_ip, "Dest IP to match");

static char *source_ip = "0.0.0.0";
module_param(source_ip, charp, 0600);
MODULE_PARM_DESC(source_ip, "Dest IP to match");

static unsigned int destination_port = 65432;
module_param(destination_port, uint, 0600);
MODULE_PARM_DESC(destination_port, "DPORT to match");

static unsigned int source_port = 0;
module_param(source_port, uint, 0600);
MODULE_PARM_DESC(source_port, "SPORT to match");

static unsigned int protocol = 256; // TCP or UDP
module_param(protocol, uint, 0600);
MODULE_PARM_DESC(protocol, "L4 protocol to match");

static unsigned short applyNow = 0;
module_param(applyNow, ushort, 0600);
MODULE_PARM_DESC(applyNow, "Apply customization lookup to all sockets, not just new sockets");

unsigned short activate = 1;
module_param(activate, ushort, 0600);
MODULE_PARM_DESC(activate, "Place customization in active mode, which allows customization of messages");

unsigned short priority = 65535;
module_param(priority, ushort, 0600);
MODULE_PARM_DESC(priority, "Customization priority level used when attaching modules to socket");

// test message for this module
char cust_start[8] = "<start>";
char cust_end[6] =   "<end>";

struct customization_node *python_cust;



// Function to customize the msg sent from the application to layer 4
// @param[I] send_buf_st Pointer to the send buffer structure
// @param[I] socket_flow Pointer to the flow struct mathing cust parameters
// @pre send_buf_st->src_iter holds app message destined for Layer 4
// @post send_buf_st->src_buf holds customized message for DCA to send to Layer 4
// Additional includes for character manipulation
#include <ctype.h>

void modify_buffer_send(struct customization_buffer *send_buf_st, struct customization_flow *socket_flow)
{
    bool copy_success;
    char prefix[] = "CLIENT SAYS: ";
    char suffix[] = "; END OF MESSAGE";
    size_t prefix_size = sizeof(prefix) - 1;
    size_t suffix_size = sizeof(suffix) - 1;
    send_buf_st->copy_length = 0;

    set_module_struct_flags(send_buf_st, false);

    if (*python_cust->active_mode == 0)
    {
        send_buf_st->try_next = true;
        return;
    }

    // Calculate new length with prefix and suffix
    send_buf_st->copy_length = prefix_size + send_buf_st->length + suffix_size;

    // Ensure the buffer is large enough
    send_buf_st->buf = krealloc(send_buf_st->buf, send_buf_st->copy_length, GFP_KERNEL);
    if(send_buf_st->buf == NULL)
    {
        trace_printk("Realloc Failed\n");
        return;
    }
    send_buf_st->buf_size = send_buf_st->copy_length;

    // Copy prefix to the buffer
    memcpy(send_buf_st->buf, prefix, prefix_size);

    // Copy the original message
    copy_success = copy_from_iter_full(send_buf_st->buf + prefix_size, send_buf_st->length, send_buf_st->src_iter);
    if (!copy_success)
    {
        trace_printk("L4.5 ALERT: Failed to copy all bytes to cust buffer\n");
        send_buf_st->copy_length = 0; // Fail gracefully
    }
    else
    {
        // Invert case of the message content
        for (size_t i = prefix_size; i < send_buf_st->length + prefix_size; ++i) {
            if (islower(send_buf_st->buf[i])) {
                send_buf_st->buf[i] = toupper(send_buf_st->buf[i]);
            } else if (isupper(send_buf_st->buf[i])) {
                send_buf_st->buf[i] = tolower(send_buf_st->buf[i]);
            }
        }

        // Copy suffix to the buffer
        memcpy(send_buf_st->buf + send_buf_st->length + prefix_size, suffix, suffix_size);
    }
}





// The init function that calls the functions to register a Layer 4.5 customization
// Client will check parameters on first sendmsg
// 0 used as default to skip port or IP checks
// protocol = 256 -> match any layer 4 protocol
// * used as a wildcard for application name to match all
// @post Layer 4.5 customization registered
int __init sample_client_start(void)
{
    char thread_name[16] = "python3";
    char application_name[16] = "python3";
    int result;


    python_cust = kmalloc(sizeof(struct customization_node), GFP_KERNEL);
    if (python_cust == NULL)
    {
        trace_printk("L4.5 ALERT: client kmalloc failed\n");
        return -1;
    }

    // provide pointer for DCA to toggle active mode instead of new function
    python_cust->active_mode = &activate;

    // provide pointer for DCA to update priority instead of new function
    python_cust->cust_priority = &priority;


    // python_cust->target_flow.protocol = 17; // UDP
    // python_cust->protocol = 6; // TCP
    python_cust->target_flow.protocol = (u16)protocol; // TCP and UDP
    memcpy(python_cust->target_flow.task_name_pid, thread_name, TASK_NAME_LEN);
    memcpy(python_cust->target_flow.task_name_tgid, application_name, TASK_NAME_LEN);

    // Client: no source IP or port set unless client called bind first
    python_cust->target_flow.dest_port = (u16)destination_port;
    python_cust->target_flow.source_port = (u16)source_port;

    // IP is a __be32 value
    python_cust->target_flow.dest_ip = in_aton(destination_ip);
    python_cust->target_flow.source_ip = in_aton(source_ip);

    // These functions must be defined and will be used to modify the
    // buffer on a send/receive call
    python_cust->send_function = modify_buffer_send;
    python_cust->recv_function = modify_buffer_recv;

    python_cust->send_buffer_size = 2048; // we don't need a full buffer
    python_cust->recv_buffer_size = 32;   // we don't plan to use this buffer

    // Cust ID normally set by NCO, uniqueness required
    python_cust->cust_id = 42;
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
MODULE_AUTHOR("Alexander Evans");
MODULE_LICENSE("GPL");
