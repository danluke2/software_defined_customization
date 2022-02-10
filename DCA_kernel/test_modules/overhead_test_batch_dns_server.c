// @file test_modules/overhead_dns_tag_server.c
// @brief The customization module to remove dns tag to test overhead

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/uio.h> // For iter structures

#include "../common_structs.h"


extern int register_customization(struct customization_node *cust);

extern int unregister_customization(struct customization_node *cust);

extern void trace_print_hex_dump(const char *prefix_str, int prefix_type, int rowsize, int groupsize,
		                                  const void *buf, size_t len, bool ascii);


char cust_tag_test[33] = "XTAGTAGTAGTAGTAGTAGTAGTAGTAGTAGX";
size_t cust_tag_test_size = (size_t)sizeof(cust_tag_test)-1; // i.e., 32 bytes


struct customization_node *dns_cust;



void modify_buffer_send(struct iov_iter *src_iter, struct customization_buffer *send_buf_st, size_t length, size_t *copy_length)
{
  copy_length = 0;
  return;
}


//reconstruct DNS packet from client side: ID+full query section->fill in header
//recvmsg_ret is the amount of data in src_buf put there by layer 4
//copy_length is amount of data in recv_buf to copy to msg
//length = max buffer length -> copy_length must be <= length
void modify_buffer_recv(struct iov_iter *src_iter, struct customization_buffer *recv_buf_st, int length, size_t recvmsg_ret,
                       size_t *copy_length)
{
  bool copy_success;
  *copy_length = 0;

  // trace_print_hex_dump("Cust DNS packet recv: ", DUMP_PREFIX_ADDRESS, 16, 1, src_iter->iov->iov_base, recvmsg_ret, true);

  if (recvmsg_ret - cust_tag_test_size > 0)
  {
    iov_iter_advance(src_iter, cust_tag_test_size);
    copy_success = copy_from_iter_full(recv_buf_st->buf, recvmsg_ret - cust_tag_test_size, src_iter);
    if(copy_success == false)
    {
      trace_printk("Failed to copy DNS to cust buffer\n");
      //Scenario 1: keep cust loaded and allow normal msg to be sent
      *copy_length = 0;
      return;
    }
    *copy_length = recvmsg_ret - cust_tag_test_size;

    // trace_print_hex_dump("DNS packet recv: ", DUMP_PREFIX_ADDRESS, 16, 1, recv_buf_st->buf, *copy_length, true);
  }

  else
  {
    //something strange came in
    trace_printk("L4.5 ALERT: DNS packet length makes no sense, size = %lu\n", recvmsg_ret);
  }

  return;
}


// The init function that calls the functions to register a Layer 4.5 customization
// @post Layer 4.5 customization registered
int __init sample_client_start(void)
{
  int result;
  char application_name[16] = "dnsmasq";

	dns_cust = kmalloc(sizeof(struct customization_node), GFP_KERNEL);
	if(dns_cust == NULL)
	{
		trace_printk("L4.5 ALERT: server dns kmalloc failed\n");
		return -1;
	}

	dns_cust->target_flow.protocol = 17; // UDP
	memcpy(dns_cust->target_flow.task_name, application_name, TASK_NAME_LEN);

	dns_cust->target_flow.dest_port = 53;
  // dnsmasq doesn't bind unless you force it, which I do
  dns_cust->target_flow.dest_ip = in_aton("10.0.0.20");
  dns_cust->target_flow.source_ip = in_aton("10.0.0.10");
  dns_cust->target_flow.source_port = 0;

	dns_cust->send_function = NULL;
	dns_cust->recv_function = modify_buffer_recv;

	dns_cust->cust_id = 65;
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

  trace_printk("L4.5: server dns module loaded, id=%d\n", dns_cust->cust_id);

  return 0;
}

// The end function that calls the functions to unregister and stop Layer 4.5
// @post Layer 4.5 customization unregistered
void __exit sample_client_end(void)
{
  int ret = unregister_customization(dns_cust);

  if(ret == 0){
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
