// @file test_modules/overhead_dns_tag_server.c
// @brief The customization module to remove dns tag to test overhead

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



extern int register_customization(struct customization_node *cust);

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

static unsigned int protocol = 17; // TCP or UDP
module_param(protocol, uint, 0600);
MODULE_PARM_DESC(protocol, "L4 protocol to match");


char cust_tag_test[33] = "XTAGTAGTAGTAGTAGTAGTAGTAGTAGTAGX";
size_t cust_tag_test_size = (size_t)sizeof(cust_tag_test) - 1; // i.e., 32 bytes


struct customization_node *dns_cust;


static size_t buffered_data = 0;  // amount of data buffered after last recv
static size_t buffered_index = 0; // index where buffered data is
static size_t insert_index = 0;   // where to insert new data


void modify_buffer_send(struct customization_buffer *send_buf_st, struct customization_flow *socket_flow)
{
    send_buf_st->copy_length = 0;
    return;
}


// remove the 32 byte tag from the DNS message
// recvmsg_ret is the amount of data in src_buf put there by layer 4
// copy_length is amount of data in recv_buf to copy to msg
// length = max buffer length -> copy_length must be <= length
void modify_buffer_recv(struct customization_buffer *recv_buf_st, struct customization_flow *socket_flow)
{
    recv_buf_st->no_cust = false;
    recv_buf_st->skip_cust = false;
    insert_index = 0;

    // we aren't buffering, so if recvmsg returned error, just pass back to app
    if (recv_buf_st->recv_return < 0)
    {
        return;
    }

    // if data was buffered in previous recv, then shift it to front of buffer
    if (buffered_data > 0)
    {
        memmove(recv_buf_st->buf, recv_buf_st->buf + buffered_index, buffered_data);
        insert_index += buffered_data;
        buffered_data = 0;
        buffered_index = 0;
        // trace_print_hex_dump("Buffered Message: ", DUMP_PREFIX_ADDRESS, 16, 1, recv_buf_st->buf, insert_index, true);
    }


    // trace_printk("L4.5 module: recvmsg_ret = %lu, msg len = %lu\n", recv_buf_st->recv_return, recv_buf_st->length);


    // NOTE: when buffering allowed, recv_return can be 0
    // We should never have buffered data for this cust module
    if (recv_buf_st->recv_return == 0)
    {
        // if we have data buffered, then give to app
        if (insert_index > 0)
        {
            if (insert_index <= recv_buf_st->length)
            {
                recv_buf_st->copy_length = insert_index;
            }
            else
            {
                // give as much as we can
                recv_buf_st->copy_length = recv_buf_st->length;
                buffered_data = recv_buf_st->length - insert_index;
                buffered_index = recv_buf_st->length;
            }
        }
        else
        {
            recv_buf_st->copy_length = 0;
        }
        return;
    }


    // trace_print_hex_dump("Temp Buffer Cust DNS packet recv: ", DUMP_PREFIX_ADDRESS, 16, 1, recv_buf_st->temp_buf,
    //                      recv_buf_st->recv_return, true);

    if (recv_buf_st->recv_return - cust_tag_test_size > 0)
    {
        if (strncmp(cust_tag_test, recv_buf_st->temp_buf, cust_tag_test_size) == 0)
        {
            // trace_printk("L4.5: cust tag found\n");
            recv_buf_st->copy_length = recv_buf_st->recv_return - cust_tag_test_size;

            if (recv_buf_st->copy_length > recv_buf_st->length)
            {
                // copy > length, so just give what we can since we don't want to buffer DNS requests
                recv_buf_st->copy_length = recv_buf_st->length;
            }

            memcpy(recv_buf_st->buf, recv_buf_st->temp_buf + cust_tag_test_size, recv_buf_st->copy_length);
        }
        else
        {
            trace_printk("L4.5: no cust tag\n");
            if (recv_buf_st->recv_return <= recv_buf_st->length)
            {
                recv_buf_st->copy_length = recv_buf_st->recv_return;
            }
            else
            {
                recv_buf_st->copy_length = recv_buf_st->length;
            }

            memcpy(recv_buf_st->buf, recv_buf_st->temp_buf, recv_buf_st->copy_length);
        }
    }
    else
    {
        // something strange came in
        trace_printk("L4.5 ALERT: DNS packet length makes no sense, size = %lu\n", recv_buf_st->recv_return);

        if (recv_buf_st->recv_return <= recv_buf_st->length)
        {
            recv_buf_st->no_cust = true;
        }
        else
        {
            // recv > length but does not match our cust, so drop the message
            recv_buf_st->copy_length = 0;
        }
    }

    // trace_print_hex_dump("Cust Buffer DNS packet recv: ", DUMP_PREFIX_ADDRESS, 16, 1, recv_buf_st->buf,
    //                      recv_buf_st->copy_length, true);


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

    dns_cust->target_flow.protocol = (u16)protocol; // UDP
    memcpy(dns_cust->target_flow.task_name_pid, thread_name, TASK_NAME_LEN);
    memcpy(dns_cust->target_flow.task_name_tgid, application_name, TASK_NAME_LEN);

    dns_cust->target_flow.dest_port = (u16)destination_port;
    // dnsmasq doesn't bind unless you force it, which I do
    dns_cust->target_flow.dest_ip = in_aton(destination_ip);
    dns_cust->target_flow.source_ip = in_aton(source_ip);
    dns_cust->target_flow.source_port = (u16)source_port;

    dns_cust->send_function = NULL;
    dns_cust->recv_function = modify_buffer_recv;

    dns_cust->cust_id = 65;
    dns_cust->registration_time_struct.tv_sec = 0;
    dns_cust->registration_time_struct.tv_nsec = 0;
    dns_cust->retired_time_struct.tv_sec = 0;
    dns_cust->retired_time_struct.tv_nsec = 0;

    dns_cust->send_buffer_size = 0;    // accept default buffer size
    dns_cust->recv_buffer_size = 8192; // we don't need a full buffer
    dns_cust->temp_buffer_size = 4096; // half of recv

    result = register_customization(dns_cust);

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
