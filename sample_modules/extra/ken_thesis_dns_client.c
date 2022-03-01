//Client DNS will capture all client DNS traffic to the specified IP address
//and truncate according to Ken's thesis (mostly), but to be simple the server
//side will respond with a normal DNS response

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/uio.h> // For iter structures

#include "../common_structs.h"

static int __init sample_client_start(void);
static void __exit sample_client_end(void);


extern int register_customization(struct customization_node *cust);

extern int unregister_customization(struct customization_node *cust);

extern void trace_print_hex_dump(const char *prefix_str, int prefix_type, int rowsize, int groupsize,
		                                  const void *buf, size_t len, bool ascii);



//DNS header structure
struct dns_header
{
	u_int16_t id; /* a 16 bit identifier assigned by the client */
	u_int16_t qr:1;
	u_int16_t opcode:4;
	u_int16_t aa:1;
	u_int16_t tc:1;
	u_int16_t rd:1;
	u_int16_t ra:1;
	u_int16_t z:3;
	u_int16_t rcode:4;
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
void modify_buffer_send(struct iov_iter *src_iter, struct customization_buffer *send_buf_st, size_t length, size_t *copy_length)
{
  bool copy_success;
  uint8_t *start_of_fqdn = (uint8_t *) ((char *)src_iter->iov->iov_base + sizeof(struct dns_header));
  size_t total = 0;
  uint8_t *field_length = start_of_fqdn;
  *copy_length = 0;

  trace_printk("L4.5: UDP DNS lengh = %lu\n", length);
  // this dump assumes DNS packet exists entirely in iov_base buffer
  trace_print_hex_dump("Original DNS packet: ", DUMP_PREFIX_ADDRESS, 16, 1, src_iter->iov->iov_base, length, true);
  return;

  copy_success = copy_from_iter_full(send_buf_st->buf, 2, src_iter); // ID
  if(copy_success == false)
  {
    //not all bytes were copied, so new_length is wrong?
    trace_printk("L4.5 ALERT: Failed to copy DNS ID into buffer\n");
    //Scenario 1: keep cust loaded and allow normal msg to be sent
    // server will just block the message b/c not in correct form
    return;
  }

  trace_print_hex_dump("Cust DNS packet ID: ", DUMP_PREFIX_ADDRESS, 16, 1, send_buf_st->buf, 2, true);
  return;

  // determine fqdn size
  while (*field_length != 0)
  {
    total +=  1;
    field_length = start_of_fqdn + total;
  }
  total += 5; // last 5 bytes finishes name query part, but we can possibly drop

  *copy_length = total + (size_t) 2; // 2 is for the id
  trace_printk("L4.5 ALERT: DNS fqdn + id copy lengh = %lu\n", *copy_length);

  // advance iter past rest of DNS header
  iov_iter_advance(src_iter, sizeof(struct dns_header) - 2);

  copy_success = copy_from_iter_full(send_buf_st->buf + (size_t) 2, length - sizeof(struct dns_header) + 2 , src_iter);
  if(copy_success == false)
  {
    trace_printk("L4.5 ALERT: Failed to copy FQDN to cust buffer\n");
    src_iter->iov_offset -= (size_t) 2; //undo other copy success

    //Scenario 1: keep cust loaded and allow normal msg to be sent
    *copy_length = 0;
  }


  trace_print_hex_dump("Cust DNS packet: ", DUMP_PREFIX_ADDRESS, 16, 1, send_buf_st->buf, *copy_length, true);

  return;
}


//length = max buffer length -> copy_length must be <= length
void modify_buffer_recv(struct iov_iter *src_iter, struct customization_buffer *recv_buf_st, int length, size_t recvmsg_ret,
                       size_t *copy_length)
{
  copy_length = 0;
 	return;
}


// The init function that calls the functions to register a Layer 4.5 customization
// @post Layer 4.5 customization registered
int __init sample_client_start(void)
{
  int result;
	char thread_name[16] = "systemd-resolve";
	char application_name[16] = "systemd-resolve";

  dns_cust = kmalloc(sizeof(struct customization_node), GFP_KERNEL);
	if(dns_cust == NULL)
	{
		trace_printk("L4.5 ALERT: client DNS structure kmalloc failed\n");
		return -1;
	}

	dns_cust->target_flow.protocol = 17; // UDP
	memcpy(dns_cust->target_flow.task_name_pid, thread_name, TASK_NAME_LEN);
	memcpy(dns_cust->target_flow.task_name_tgid, application_name, TASK_NAME_LEN);

	dns_cust->target_flow.dest_port = 53;
  dns_cust->target_flow.dest_ip = in_aton("10.0.0.10");
  dns_cust->target_flow.source_ip = 0;
  dns_cust->target_flow.source_port = 0;

	dns_cust->send_function = modify_buffer_send;
	dns_cust->recv_function = NULL;

	dns_cust->cust_id = 34;
  dns_cust->registration_time_struct.tv_sec = 0;
  dns_cust->registration_time_struct.tv_nsec = 0;
  dns_cust->retired_time_struct.tv_sec = 0;
  dns_cust->retired_time_struct.tv_nsec = 0;

  dns_cust->send_buffer_size = 0; // accept default buffer size
  dns_cust->recv_buffer_size = 0; // accept default buffer size

	result = register_customization(dns_cust);

  if(result != 0)
  {
    trace_printk("L4.5 ALERT: Module failed registration, check debug logs\n");
    return -1;
  }

	trace_printk("L4.5: client dns module loaded, id=%d\n", dns_cust->cust_id);

  return 0;
}


// The end function that calls the functions to unregister and stop Layer 4.5
// @post Layer 4.5 customization unregistered
void __exit sample_client_end(void)
{
  int ret = unregister_customization(dns_cust);

  if(ret == 0){
    trace_printk("L4.5 ALERT: client module unload error\n");
  }
  else
  {
    trace_printk("L4.5: client module unloaded\n");
  }
  kfree(dns_cust);
	return;
}


module_init(sample_client_start);
module_exit(sample_client_end);
MODULE_AUTHOR("Dan Lukaszewski");
MODULE_LICENSE("GPL");
