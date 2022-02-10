// @file test_modules/overhead_test_app_tag_server.c
// @brief The customization module to add app tag to test overhead

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



struct customization_node *server_cust;

size_t extra_bytes_copied_from_last_send = 0;
size_t BYTE_POSIT = 1000;

size_t total_bytes_from_app = 0;
size_t total_tags = 0;

char cust_tag_test[33] = "XTAGTAGTAGTAGTAGTAGTAGTAGTAGTAGX";
size_t cust_tag_test_size = (size_t)sizeof(cust_tag_test)-1;





// Helpers
void trace_print_cust_iov_params(struct iov_iter *src_iter)
{
    trace_printk("msg iov len = %lu; offset = %lu\n", src_iter->iov->iov_len, src_iter->iov_offset);
    trace_printk("Total amount of data pointed to by the iovec array (count) = %lu\n", src_iter->count);
    trace_printk("Number of iovec structures (nr_segs) = %lu\n", src_iter->nr_segs);
}



// Function to customize the msg sent from the application to layer 4
// @param[I] src_iter Application message structure to copy from
// @param[I] send_buf_st Pointer to the send buffer structure
// @param[I] length Total size of the application message
// @param[X] copy_length Number of bytes PCM needs to send to layer 4
// @pre src_iter holds app message destined for Layer 4
// @post src_buf holds customized message for PCM to send to Layer 4
void modify_buffer_send(struct iov_iter *src_iter, struct customization_buffer *send_buf_st, size_t length, size_t *copy_length)
{
  bool copy_success;
  size_t i = 0;
  size_t remaining_length = length;
  size_t loop_length = length;
  u32 number_of_tags_added = 0;
  *copy_length = 0;

  total_bytes_from_app += length;
  // trace_printk("L4.5: Total bytes from app to cust mod = %lu\n", total_bytes_from_app);

  if (extra_bytes_copied_from_last_send > 0)
  {
    // we had bytes counting towards BYTE_POSIT from last send call
    if (BYTE_POSIT - extra_bytes_copied_from_last_send > length)
    {
      //copy entire buffer because still under BYTE_POSIT value
      copy_success = copy_from_iter_full(send_buf_st->buf, length, src_iter);
      if(copy_success == false)
      {
        trace_printk("L4.5 ALERT: Length not big enough, Failed to copy all bytes to cust buffer\n");
        trace_print_cust_iov_params(src_iter);
        return;
      }
      *copy_length += length;
      extra_bytes_copied_from_last_send += length;
      remaining_length -= length;
    }
    else
    {
      // first copy remaining bytes to reach BYTE_POSIT
      copy_success = copy_from_iter_full(send_buf_st->buf, BYTE_POSIT - extra_bytes_copied_from_last_send, src_iter);
      if(copy_success == false)
      {
        // not all bytes were copied, so pick scenario 1 or 2 below
        trace_printk("L4.5 ALERT: Length check good, Failed to copy bytes to cust buffer\n");
        // Scenario 1: keep cust loaded and allow normal msg to be sent
        *copy_length = 0;
        trace_print_cust_iov_params(src_iter);
        return;
      }
      *copy_length += BYTE_POSIT - extra_bytes_copied_from_last_send;
      remaining_length -= *copy_length;
      extra_bytes_copied_from_last_send = 0;

      // reached tag posit, so put in our generic tag
      memcpy(send_buf_st->buf + *copy_length, cust_tag_test, cust_tag_test_size);
      *copy_length += cust_tag_test_size;
      number_of_tags_added +=1;
    }
  }

  loop_length = remaining_length;
  // at this point we have 0 bytes inserted toward BYTE_POSIT tag positiion
  for (i = 0; i + BYTE_POSIT <= loop_length; i+=BYTE_POSIT)
  {
  	copy_success = copy_from_iter_full(send_buf_st->buf + *copy_length, BYTE_POSIT, src_iter);
    if(copy_success == false)
    {
      // not all bytes were copied, so pick scenario 1 or 2 below
      trace_printk("L4.5 ALERT: For loop, Failed to copy bytes to cust buffer\n");
      // Scenario 1: keep cust loaded and allow normal msg to be sent
      *copy_length = 0;
      trace_print_cust_iov_params(src_iter);
      return;
    }
    *copy_length += BYTE_POSIT;
    remaining_length -= BYTE_POSIT;

    // now insert tag
    memcpy(send_buf_st->buf + *copy_length, cust_tag_test, cust_tag_test_size);
    *copy_length += cust_tag_test_size;
    number_of_tags_added +=1;
  }


  if (remaining_length > 0)
  {
    //copy over leftover bytes from loop
    copy_success = copy_from_iter_full(send_buf_st->buf + *copy_length, remaining_length, src_iter);
    if(copy_success == false)
    {
      trace_printk("L4.5 ALERT: Failed to copy remaining bytes to cust buffer\n");
      *copy_length = 0;
      trace_print_cust_iov_params(src_iter);
      return;
    }
    extra_bytes_copied_from_last_send += remaining_length;
    *copy_length += remaining_length;
  }
  total_tags += number_of_tags_added;

  // trace_printk("L4.5: Number of tags inserted = %u, total = %lu\n", number_of_tags_added, total_tags);
	return;
}


// Function to customize the msg recieved from L4 prior to delivery to application
void modify_buffer_recv(struct iov_iter *src_iter, struct customization_buffer *recv_buf_st, int length, size_t recvmsg_ret,
                       size_t *copy_length)
{
  *copy_length = 0;
 	return;
}



// The init function that calls the functions to register a Layer 4.5 customization
// @post Layer 4.5 customization registered
int __init sample_client_start(void)
{
	char application_name[16] = "python3";
  int result;

	server_cust = kmalloc(sizeof(struct customization_node), GFP_KERNEL);
	if(server_cust == NULL)
	{
		trace_printk("L4.5 ALERT: server kmalloc failed\n");
		return -1;
	}

  server_cust->target_flow.protocol = 6; // TCP
	memcpy(server_cust->target_flow.task_name, application_name, TASK_NAME_LEN);

	server_cust->target_flow.dest_port = 8080;
  server_cust->target_flow.source_port = 0;

  server_cust->target_flow.dest_ip = in_aton("0.0.0.0");
  server_cust->target_flow.source_ip = in_aton("0.0.0.0");

	server_cust->send_function = modify_buffer_send;
	server_cust->recv_function = NULL;

	server_cust->cust_id = 87;
  server_cust->registration_time_struct.tv_sec = 0;
  server_cust->registration_time_struct.tv_nsec = 0;
	server_cust->retired_time_struct.tv_sec = 0;
  server_cust->retired_time_struct.tv_nsec = 0;

  server_cust->send_buffer_size = 65536 * 2; // bufer
  server_cust->recv_buffer_size = 0; // accept default buffer size

	result = register_customization(server_cust);

  if(result != 0)
  {
    trace_printk("L4.5 ALERT: Module failed registration, check debug logs\n");
    return -1;
  }

	trace_printk("L4.5: server module loaded, id=%d\n", server_cust->cust_id);

  return 0;
}


// Calls the functions to unregister customization node from use on sockets
// @post Layer 4.5 customization node unregistered
void __exit sample_client_end(void)
{
  //NOTE: this is only valid/safe place to call unregister (deadlock scenario)
  int ret = unregister_customization(server_cust);

  if(ret == 0){
    trace_printk("L4.5 ALERT: server module unload error\n");
  }
  else
  {
    trace_printk("L4.5: server module unloaded\n");
  }
  kfree(server_cust);
	return;
}



module_init(sample_client_start);
module_exit(sample_client_end);
MODULE_AUTHOR("Dan Lukaszewski");
MODULE_LICENSE("GPL");
