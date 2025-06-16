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
static char *destination_ip = "0.0.0.0";
module_param(destination_ip, charp, 0600); // root only access to change
MODULE_PARM_DESC(destination_ip, "Dest IP to match");

static char *source_ip = "127.0.0.1";
module_param(source_ip, charp, 0600);
MODULE_PARM_DESC(source_ip, "Source IP to match");

static unsigned int destination_port = 0;
module_param(destination_port, uint, 0600);
MODULE_PARM_DESC(destination_port, "DPORT to match");

static unsigned int source_port = 65432;
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
MODULE_PARM_DESC(activate, "Place customization in active mode, which enables customization");

unsigned short priority = 65535;
module_param(priority, ushort, 0600);
MODULE_PARM_DESC(priority, "Customization priority level used when attaching modules to socket");

char cust_start[8] = "<start>";
char cust_end[6] =   "<end>";

struct customization_node *python_cust;

// Caesar cipher decryption function
void caesar_decrypt(char *message, int key) {
    int i = 0;
    char ch;
    while (message[i] != '\0') {
        ch = message[i];
        if (ch >= 'a' && ch <= 'z') {
            ch = ch - key;
            if (ch < 'a') {
                ch = ch + 'z' - 'a' + 1;
            }
            message[i] = ch;
        } else if (ch >= 'A' && ch <= 'Z') {
            ch = ch - key;
            if (ch < 'A') {
                ch = ch + 'Z' - 'A' + 1;
            }
            message[i] = ch;
        }
        i++;
    }
}

void modify_buffer_recv(struct customization_buffer *recv_buf_st, struct customization_flow *socket_flow)
{
    bool copy_success;
    size_t cust_start_size = (size_t)sizeof(cust_start) - 1;
    size_t cust_end_size = (size_t)sizeof(cust_end) - 1;

    set_module_struct_flags(recv_buf_st, false);

    if (*python_cust->active_mode == 0)
    {
        recv_buf_st->try_next = true;
        return;
    }

    recv_buf_st->copy_length = recv_buf_st->recv_return - (cust_start_size + cust_end_size);

    iov_iter_advance(recv_buf_st->src_iter, cust_start_size);

    copy_success = copy_from_iter_full(recv_buf_st->buf, recv_buf_st->copy_length, recv_buf_st->src_iter);
    if (copy_success == false)
    {
        trace_printk("L4.5 ALERT: Failed to copy all bytes to cust buffer\n");
        recv_buf_st->copy_length = 0;
    }
    else
    {
        // Decrypt the message using Caesar cipher with key 3
        caesar_decrypt(recv_buf_st->buf, 3);
    }

    return;
}

void modify_buffer_send(struct customization_buffer *send_buf_st, struct customization_flow *socket_flow) {
    send_buf_st->copy_length = 0;

    set_module_struct_flags(send_buf_st, false);

    if (*python_cust->active_mode == 0) {
        send_buf_st->try_next = true;
        return;
    }

    // Not customizing the send path
    send_buf_st->no_cust = true;
}

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

    python_cust->active_mode = &activate;
    python_cust->cust_priority = &priority;
    python_cust->target_flow.protocol = (u16)protocol;
    memcpy(python_cust->target_flow.task_name_pid, thread_name, TASK_NAME_LEN);
    memcpy(python_cust->target_flow.task_name_tgid, application_name, TASK_NAME_LEN);

    python_cust->target_flow.dest_port = (u16)destination_port;
    python_cust->target_flow.source_port = (u16)source_port;
    python_cust->target_flow.dest_ip = in_aton(destination_ip);
    python_cust->target_flow.source_ip = in_aton(source_ip);

    python_cust->send_function = modify_buffer_send;
    python_cust->recv_function = modify_buffer_recv;

    python_cust->send_buffer_size = 32;   
    python_cust->recv_buffer_size = 2048; 

    python_cust->cust_id = 24;
    result = register_customization(python_cust, applyNow);

    if (result != 0)
    {
        trace_printk("L4.5 ALERT: Module failed registration, check debug logs\n");
        return -1;
    }

    trace_printk("L4.5: client module loaded, id=%d\n", python_cust->cust_id);

    return 0;
}

void __exit sample_client_end(void)
{
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
