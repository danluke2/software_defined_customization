// @file core_modules/NCO_test.c
// @brief Test module to verify challenge-response works

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/inet.h>
#include <linux/uio.h> // For iter structures

// for the overhead test, a copy of common_structs is in same location as this module
#include "common_structs.h"


extern int register_customization(struct customization_node *cust);

extern int unregister_customization(struct customization_node *cust);

//test message for this plugin
char cust_test[12] = "testCustMod";
size_t cust_test_size = (size_t)sizeof(cust_test)-1;

struct customization_node *python_cust;















// Line 42 should be blank b/c NCO will write the module_id variable to that line
// followed by any other variables we determine NCO should declare when building



// Function to customize the msg sent from the application to layer 4
// @param[I] src_iter Application message structure to copy from
// @param[I] send_buf_st Pointer to the send buffer structure
// @param[I] length Total size of the application message
// @param[X] copy_length Number of bytes DCA needs to send to layer 4
// @pre src_iter holds app message destined for Layer 4
// @post src_buf holds customized message for DCA to send to Layer 4
void modify_buffer_send(struct iov_iter *src_iter, struct customization_buffer *send_buf_st, size_t length, size_t *copy_length)
{
  bool copy_success;

	*copy_length = cust_test_size + length;

	memcpy(send_buf_st->buf, cust_test, cust_test_size);
  //copy from full will revert iter back to normal if failure occurs
	copy_success = copy_from_iter_full(send_buf_st->buf+cust_test_size, length, src_iter);
  if(copy_success == false)
  {
    // not all bytes were copied, so pick scenario 1 or 2 below
    trace_printk("Failed to copy all bytes to cust buffer\n");
    copy_length = 0;
  }
	return;
}


// Function to customize the msg recieved from L4 prior to delivery to application
// @param[I] src_iter Application message structure to copy from
// @param[I] recv_buf_st Pointer to the recv buffer structure
// @param[I] length Max bytes application can receive;
// @param[I] recvmsg_ret Total size of message in src_iter
// @param[X] copy_length Number of bytes DCA needs to send to application
// @pre src_iter holds app message destined for application
// @post recv_buf holds customized message for DCA to send to app instead
// NOTE: copy_length must be <= length
void modify_buffer_recv(struct iov_iter *src_iter, struct customization_buffer *recv_buf_st, int length, size_t recvmsg_ret,
                       size_t *copy_length)
{
  bool copy_success;

 	*copy_length = recvmsg_ret - cust_test_size;

  //adjust iter offset to start of actual message, then copy
  iov_iter_advance(src_iter, cust_test_size);

  copy_success = copy_from_iter_full(recv_buf_st->buf, *copy_length, src_iter);
  if(copy_success == false)
  {
    // not all bytes were copied, so pick scenario 1 or 2 below
    trace_printk("Failed to copy all bytes to cust buffer\n");
    *copy_length = 0;
  }
 	return;
}



// The init function that calls the functions to register a Layer 4.5 customization
// Client will check parameters on first sendmsg
// 0 used as default to skip port or IP checks
// protocol = 256 -> match any layer 4 protocol
// * used as a wildcard for application name to match all [TODO: test]
// @post Layer 4.5 customization registered
int __init sample_client_start(void)
{
	char application_name[16] = "python3";
  int result;


	python_cust = kmalloc(sizeof(struct customization_node), GFP_KERNEL);
	if(python_cust == NULL)
	{
		trace_printk("client kmalloc failed\n");
		return -1;
	}

	python_cust->target_flow.protocol = 17; // UDP
  // python_cust->protocol = 6; // TCP
  // python_cust->protocol = 256; // Any since not valid IP field value
	memcpy(python_cust->target_flow.task_name, application_name, TASK_NAME_LEN);

  // Client: no source IP or port set unless client called bind first
	python_cust->target_flow.dest_port = 65432;
  python_cust->target_flow.source_port = 0;

  //IP is a __be32 value
  python_cust->target_flow.dest_ip = in_aton("127.0.0.1");
  python_cust->target_flow.source_ip = 0;

  // These functions must be defined and will be used to modify the
  // buffer on a send/receive call
  // Function can be set to NULL if not modifying buffer contents
	python_cust->send_function = modify_buffer_send;
	python_cust->recv_function = NULL;

  // Cust ID set by customization controller, network uniqueness required
	python_cust->cust_id = module_id;
  python_cust->registration_time_struct.tv_sec = 0;
  python_cust->registration_time_struct.tv_nsec = 0;
	python_cust->retired_time_struct.tv_sec = 0;
  python_cust->retired_time_struct.tv_nsec = 0;

	result = register_customization(python_cust);

  if(result != 0)
  {
    trace_printk("L4.5 AELRT: Module failed registration, check debug logs\n");
    return -1;
  }

	trace_printk("L4.5: client module loaded, id=%d\n", python_cust->cust_id);

  return 0;
}


// Calls the functions to unregister customization node from use on sockets
// @post Layer 4.5 customization node unregistered
void __exit sample_client_end(void)
{
  //NOTE: this is only valid/safe place to call unregister (deadlock scenario)
  int ret = unregister_customization(python_cust);

  if(ret == 0){
    trace_printk("L4.5 AELRT: client module unload error\n");
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