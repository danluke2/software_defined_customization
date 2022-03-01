// @file sample_python_server.c
// @brief A sample customization module to modify python3 send/recv calls

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/inet.h>
#include <linux/uio.h> // For iter structures

#include "../DCA_kernel/common_structs.h"


extern int register_customization(struct customization_node *cust);

extern int unregister_customization(struct customization_node *cust);


//test message for this plugin
char cust_test[12] = "testCustMod";
//-1 b/c don't want terminating part
size_t cust_test_size = (size_t)sizeof(cust_test)-1;

struct customization_node *nginx_cust;





// this just does a straight copy operation
void modify_buffer_send(struct iov_iter *src_iter, struct customization_buffer *send_buf_st, size_t length, size_t *copy_length)
{
  bool copy_success;
  trace_printk("L4.5: TLS send buffer size %lu\n", length);

	*copy_length = length;

  //copy from full will revert iter back to normal if failure occurs
	copy_success = copy_from_iter_full(send_buf_st->buf, length, src_iter);
  if(copy_success == false)
  {
    // not all bytes were copied, so pick scenario 1 or 2 below
    trace_printk("L4.5 ALERT: Failed to copy all bytes to cust buffer\n");
    // Scenario 1: keep cust loaded and allow normal msg to be sent
    copy_length = 0;

  }
	return;
}


void modify_buffer_recv(struct iov_iter *src_iter, struct customization_buffer *recv_buf_st, int length, size_t recvmsg_ret, size_t *copy_length)
{
  bool copy_success;

  trace_printk("L4.5: TLS recv ret %lu, buffer limit %d\n", recvmsg_ret, length);

 	*copy_length = recvmsg_ret;

  copy_success = copy_from_iter_full(recv_buf_st->buf, *copy_length, src_iter);
  if(copy_success == false)
  {
    // not all bytes were copied, so pick scenario 1 or 2 below
    trace_printk("L4.5 ALERT: Failed to copy all bytes to cust buffer\n");
    // Scenario 1: keep cust loaded and allow normal msg to be sent
    *copy_length = 0;
  }
 	return;
}


// The init function that calls the functions to register a Layer 4.5 customization
// Server will check parameters on first recvmsg
// 0 used as default to skip port or IP checks
// protocol = 256 -> match any layer 4 protocol
// * used as a wildcard for application name to match all [TODO: test]
// @post Layer 4.5 customization registered
int __init sample_server_start(void)
{
	//exact application name (i.e., will use strcmp function)
	char thread_name[16] = "nginx"; //16 b/c size of kernel char array
  char application_name[16] = "nginx"; //16 b/c size of kernel char array
  int result;

	nginx_cust = kmalloc(sizeof(struct customization_node), GFP_KERNEL);
	if(nginx_cust == NULL)
	{
		trace_printk("L4.5 ALERT: server kmalloc failed\n");
		return -1;
	}

  nginx_cust->target_flow.protocol = 6; //TCP
	memcpy(nginx_cust->target_flow.task_name_pid, thread_name, TASK_NAME_LEN);
  memcpy(nginx_cust->target_flow.task_name_tgid, application_name, TASK_NAME_LEN);

  // Server: source IP or port set b/c bind is called at setup
	nginx_cust->target_flow.dest_port = 0; // set if you know client port
  nginx_cust->target_flow.source_port = 443;

  //IP is a __be32 value
  nginx_cust->target_flow.dest_ip = in_aton("10.0.0.40");
  nginx_cust->target_flow.source_ip = in_aton("10.0.0.20");

	nginx_cust->send_function = modify_buffer_send;
	nginx_cust->recv_function = modify_buffer_recv;

  // Cust ID normally set by NCO, uniqueness required
	nginx_cust->cust_id = 24;

	result = register_customization(nginx_cust);

  if(result != 0)
  {
    trace_printk("L4.5 ALERT: Module failed registration, check debug logs\n");
    return -1;
  }

	trace_printk("L4.5: server module loaded, id=%d\n", nginx_cust->cust_id);

  return 0;
}

// Calls the functions to unregister customization node from use on sockets
// @post Layer 4.5 customization node unregistered
void __exit sample_server_end(void)
{
  int ret = unregister_customization(nginx_cust);

  if(ret == 0)
  {
    trace_printk("L4.5 ALERT: server module unload error\n");
  }
  else
  {
    trace_printk("L4.5: server module unloaded\n");
  }
  kfree(nginx_cust);
	return;
}

module_init(sample_server_start);
module_exit(sample_server_end);
MODULE_AUTHOR("Dan Lukaszewski");
MODULE_LICENSE("GPL");
