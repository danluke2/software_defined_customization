// Client DNS will capture all client DNS traffic to the specified IP address
// and truncate according to Ken's thesis (mostly), but to be simple the server
// side will respond with a normal DNS response

#include <linux/inet.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uio.h> // For iter structures

// ************** STANDARD PARAMS MUST GO HERE ****************
#include "../common_structs.h"
// ************** END STANDARD PARAMS ****************

extern int register_customization(struct customization_node *cust, bool applyNow);

extern int unregister_customization(struct customization_node *cust);


// Kernel module parameters with default values
static char *destination_ip = "10.10.0.1";
module_param(destination_ip, charp, 0600); // root only access to change
MODULE_PARM_DESC(destination_ip, "Dest IP to match");

static char *source_ip = "10.10.0.3";
module_param(source_ip, charp, 0600);
MODULE_PARM_DESC(source_ip, "Dest IP to match");

static unsigned int destination_port = 0;
module_param(destination_port, uint, 0600);
MODULE_PARM_DESC(destination_port, "DPORT to match");

static unsigned int source_port = 53;
module_param(source_port, uint, 0600);
MODULE_PARM_DESC(source_port, "SPORT to match");

static unsigned int protocol = 17; //  UDP
module_param(protocol, uint, 0600);
MODULE_PARM_DESC(protocol, "L4 protocol to match");



// NCO VARIABLES GO HERE
u16 module_id = 1;
char hex_key[HEX_KEY_LENGTH] = "";
u16 activate = 0;
u16 priority = 0;
u16 applyNow = 0;


// END NCO VARIABLES


// DNS header structure
struct dns_header
{
    // u_int16_t id; /* a 16 bit identifier assigned by the client */
    u_int16_t qr : 1;
    u_int16_t opcode : 4;
    u_int16_t aa : 1;
    u_int16_t tc : 1;
    u_int16_t rd : 1;
    u_int16_t ra : 1;
    u_int16_t z : 3;
    u_int16_t rcode : 4;
    u_int16_t qdcount;
    u_int16_t ancount;
    u_int16_t nscount;
    u_int16_t arcount;
};


struct dns_packet
{
    struct dns_header header;
    //	struct dns_question question;
    char *data;
    u_int16_t data_size;
};


struct customization_node *dns_cust;



// The following functions perform the buffer modifications requested by handler
// send_buf could be realloc and change, thus update buf ptr and size if necessary
void modify_buffer_send(struct customization_buffer *send_buf_st, struct customization_flow *socket_flow)
{
    send_buf_st->no_cust = false;
    send_buf_st->set_cust_to_skip = false;
    send_buf_st->try_next = false;

    // must pass active_mode check to customize
    if (*dns_cust->active_mode == 0)
    {
        send_buf_st->try_next = true;
        return;
    }


    send_buf_st->no_cust = true;
    return;
}


// reconstruct DNS packet from client side: ID+full query section->fill in header
// recv_return is the amount of data in src_buf put there by layer 4
// copy_length is amount of data in recv_buf to copy to msg
// length = max buffer length -> copy_length must be <= length
void modify_buffer_recv(struct customization_buffer *recv_buf_st, struct customization_flow *socket_flow)
{
    bool copy_success;
    char *dns_buf[17]; // just adding some extra room here, only need 10 bytes, 12 with ID
    struct dns_header *dns = (struct dns_header *)dns_buf;

    recv_buf_st->no_cust = false;
    recv_buf_st->set_cust_to_skip = false;
    recv_buf_st->try_next = false;

    // must pass active_mode check to customize
    if (*dns_cust->active_mode == 0)
    {
        recv_buf_st->try_next = true;
        return;
    }

    // we should do a check here to see if module matches
    // simple length check suffices for now
    if (recv_buf_st->recv_return > 32)
    {
        // something else came in
        trace_printk("L4.5: DNS packet does not match compression module, size = %lu\n", recv_buf_st->recv_return);
        recv_buf_st->try_next = true;
        return;
    }

    recv_buf_st->copy_length = recv_buf_st->recv_return + (size_t)10;

    dns->qr = 0;     // This is a query
    dns->opcode = 0; // This is a standard query
    dns->aa = 0;     // Not Authoritative
    dns->tc = 0;     // This message is not truncated
    // flopped rd and ra for testing bit ordering at server
    dns->rd = 0; // Recursion Desired
    dns->ra = 1; // Recursion not available
    dns->z = 0;
    dns->rcode = 0;
    dns->qdcount = htons(1); // we have only 1 question
    dns->ancount = 0;
    dns->nscount = 0;
    dns->arcount = 0;


    // copy first 2 bytes which is the DNS ID
    copy_success = copy_from_iter_full(recv_buf_st->buf, 2, recv_buf_st->src_iter);
    if (copy_success == false)
    {
        trace_printk("L4.5 ALERT: Failed to copy DNS ID to cust buffer\n");
        // Scenario 1: keep cust loaded and allow normal msg to be sent
        recv_buf_st->copy_length = 0;
        return;
    }

    memcpy(recv_buf_st->buf + 2, dns_buf, 10); // insert rest of dns header




    // copy query portion
    copy_success = copy_from_iter_full(recv_buf_st->buf + 12, recv_buf_st->recv_return - 2, recv_buf_st->src_iter);
    if (copy_success == false)
    {
        trace_printk("L4.5 ALERT: Failed to copy FQDN to cust buffer\n");
        recv_buf_st->src_iter->iov_offset -= (size_t)2; // undo other copy success
        // Scenario 1: keep cust loaded and allow normal msg to be sent
        recv_buf_st->copy_length = 0;
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

    // provide pointer for DCA to toggle active mode instead of new function
    dns_cust->active_mode = &activate;

    // provide pointer for DCA to update priority instead of new function
    dns_cust->cust_priority = &priority;

    dns_cust->target_flow.protocol = (u16)protocol; // UDP
    memcpy(dns_cust->target_flow.task_name_pid, thread_name, TASK_NAME_LEN);
    memcpy(dns_cust->target_flow.task_name_tgid, application_name, TASK_NAME_LEN);

    dns_cust->target_flow.dest_port = (u16)destination_port;

    // dnsmasq doesn't bind unless you force it
    dns_cust->target_flow.dest_ip = in_aton(destination_ip);
    dns_cust->target_flow.source_ip = in_aton(source_ip);
    dns_cust->target_flow.source_port = (u16)source_port;

    dns_cust->send_function = modify_buffer_send;
    dns_cust->recv_function = modify_buffer_recv;


    dns_cust->cust_id = module_id;
    dns_cust->registration_time_struct.tv_sec = 0;
    dns_cust->registration_time_struct.tv_nsec = 0;
    dns_cust->revoked_time_struct.tv_sec = 0;
    dns_cust->revoked_time_struct.tv_nsec = 0;

    dns_cust->send_buffer_size = 0; // accept default buffer size
    dns_cust->recv_buffer_size = 0; // accept default buffer size

    result = register_customization(dns_cust, applyNow);

    if (result != 0)
    {
        trace_printk("L4.5 ALERT: Module failed registration, check debug logs\n");
        return -1;
    }

    trace_printk("L4.5: server compression dns module loaded, id=%d\n", dns_cust->cust_id);

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
