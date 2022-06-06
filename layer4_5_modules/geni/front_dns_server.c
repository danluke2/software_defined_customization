// @file test_modules/demo_dns_app_tag.c
// @brief The customization module to remove dns app tag for demo

#include <linux/inet.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uio.h> // For iter structures
#include <linux/version.h>

// ************** STANDARD PARAMS MUST GO HERE ****************
#include <common_structs.h>
#include <printing.h>
// ************** END STANDARD PARAMS ****************


static int __init sample_client_start(void);
static void __exit sample_client_end(void);

extern int register_customization(struct customization_node *cust, u16 applyNow);

extern int unregister_customization(struct customization_node *cust);

extern void trace_print_hex_dump(const char *prefix_str, int prefix_type, int rowsize, int groupsize, const void *buf,
                                 size_t len, bool ascii);

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

unsigned short activate = 1;
module_param(activate, ushort, 0600);
MODULE_PARM_DESC(activate, "Place customization in active mode, which enables customization");

char cust_tag_test[9] = "XTAGXdig";

struct customization_node *dns_cust;

u16 module_id = 2;




void modify_buffer_send(struct customization_buffer *send_buf_st, struct customization_flow *socket_flow)
{
    send_buf_st->copy_length = 0;
    send_buf_st->no_cust = false;
    send_buf_st->set_cust_to_skip = false;

    // if module hasn't been activated, then don't perform customization
    if (*dns_cust->active_mode == 0)
    {
        send_buf_st->no_cust = true;
        return;
    }

    send_buf_st->no_cust = true;
    return;
}



void modify_buffer_recv(struct customization_buffer *recv_buf_st, struct customization_flow *socket_flow)
{
    bool copy_success;
    char tag[6] = "XTAGX";
    size_t cust_tag_test_size = (size_t)sizeof(cust_tag_test) - 1; // i.e., 8 bytes
    recv_buf_st->no_cust = false;
    recv_buf_st->set_cust_to_skip = false;

    // if module hasn't been activated, then don't perform customization
    if (*dns_cust->active_mode == 0)
    {
        recv_buf_st->no_cust = true;
        return;
    }


    recv_buf_st->copy_length = 0;

    // trace_print_hex_dump("Cust DNS packet recv: ", DUMP_PREFIX_ADDRESS, 16, 1, recv_buf_st->src_iter->iov->iov_base,
    // recv_buf_st->recv_return, true);

    if (strncmp((char *)recv_buf_st->src_iter->iov->iov_base, tag, 5) == 0)
    {
        iov_iter_advance(recv_buf_st->src_iter, cust_tag_test_size);
        copy_success =
            copy_from_iter_full(recv_buf_st->buf, recv_buf_st->recv_return - cust_tag_test_size, recv_buf_st->src_iter);
        if (copy_success == false)
        {
            trace_printk("L4.5 ALERT: Failed to copy DNS to cust buffer\n");
            // Scenario 1: keep cust loaded and allow normal msg to be sent
            recv_buf_st->copy_length = 0;
            return;
        }
        recv_buf_st->copy_length = recv_buf_st->recv_return - cust_tag_test_size;

        // trace_print_hex_dump("DNS packet recv: ", DUMP_PREFIX_ADDRESS, 16, 1, recv_buf_st->buf,
        // recv_buf_st->copy_length, true);
    }

    else
    {
        // something strange came in
        trace_printk("L4.5: DNS packet does not match custom pattern, size = %lu\n", recv_buf_st->recv_return);
    }

    return;
}



// The init function that calls the functions to register a Layer 4.5 customization
// @post Layer 4.5 customization registered
int __init sample_client_start(void)
{
    int result;
    char thread_name[16] = "dnsmasq";
    char application_name[16] = "dnsmasq";

    dns_cust = kmalloc(sizeof(struct customization_node), GFP_KERNEL);
    if (dns_cust == NULL)
    {
        trace_printk("L4.5 ALERT: server dns kmalloc failed\n");
        return -1;
    }

    dns_cust->active_mode = &activate;

    dns_cust->target_flow.protocol = protocol;
    memcpy(dns_cust->target_flow.task_name_pid, thread_name, TASK_NAME_LEN);
    memcpy(dns_cust->target_flow.task_name_tgid, application_name, TASK_NAME_LEN);

    dns_cust->target_flow.dest_port = (u16)destination_port;
    // dnsmasq doesn't bind unless you force it, which I do
    dns_cust->target_flow.dest_ip = in_aton(destination_ip);
    dns_cust->target_flow.source_ip = in_aton(source_ip);
    dns_cust->target_flow.source_port = (u16)source_port;

    dns_cust->send_function = modify_buffer_send;
    dns_cust->recv_function = modify_buffer_recv;

    dns_cust->send_buffer_size = 32;
    dns_cust->recv_buffer_size = 4096;

    // Cust ID set by customization controller, network uniqueness required
    dns_cust->cust_id = module_id;
    dns_cust->registration_time_struct.tv_sec = 0;
    dns_cust->registration_time_struct.tv_nsec = 0;
    dns_cust->revoked_time_struct.tv_sec = 0;
    dns_cust->revoked_time_struct.tv_nsec = 0;

    result = register_customization(dns_cust, applyNow);

    if (result != 0)
    {
        trace_printk("L4.5 ALERT: Module failed registration, check debug logs\n");
        return -1;
    }

    trace_printk("L4.5: server dns module loaded, id=%d\n", dns_cust->cust_id);

    return 0;
}


// The end function that calls the functions to unregister and stop Layer 4.5
// @post Layer 4.5 customization unregistered
void __exit sample_client_end(void)
{
    int ret = unregister_customization(dns_cust);

    if (ret == 0)
    {
        trace_printk("L4.5 ALERT: server module unload error\n");
    }
    else
    {
        trace_printk("L4.5: server module unloaded\n");
    }
    kfree(dns_cust);
    return;
}


module_init(sample_client_start);
module_exit(sample_client_end);
MODULE_AUTHOR("Dan Lukaszewski");
MODULE_LICENSE("GPL");
