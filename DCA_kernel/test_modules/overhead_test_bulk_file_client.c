// @file test_modules/overhead_test_app_tag_client.c
// @brief The customization module to remove app tag to test overhead

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/inet.h>
#include <linux/uio.h> // For iter structures

#include "../common_structs.h"

static int __init sample_client_start(void);
static void __exit sample_client_end(void);


extern int register_customization(struct customization_node *cust);

extern int unregister_customization(struct customization_node *cust);

extern void trace_print_hex_dump(const char *prefix_str, int prefix_type, int rowsize, int groupsize,
		                                  const void *buf, size_t len, bool ascii);


size_t extra_bytes_copied_from_last_send = 0;
size_t tag_bytes_removed_last_round = 0;

size_t total_bytes_from_server = 0;
size_t app_bytes_from_server = 0;
size_t total_tags = 0;

size_t BYTE_POSIT = 1000;

char cust_tag_test[33] = "XTAGTAGTAGTAGTAGTAGTAGTAGTAGTAGX";
size_t cust_tag_test_size = (size_t)sizeof(cust_tag_test)-1; // i.e., 32 bytes

struct customization_node *client_cust;


// Helpers
void trace_print_cust_iov_params(struct iov_iter *src_iter)
{
    trace_printk("msg iov len = %lu; offset = %lu\n", src_iter->iov->iov_len, src_iter->iov_offset);
    trace_printk("Total amount of data pointed to by the iovec array (count) = %lu\n", src_iter->count);
    trace_printk("Number of iovec structures (nr_segs) = %lu\n", src_iter->nr_segs);
}


// Function to customize the msg sent from the application to layer 4
void modify_buffer_send(struct iov_iter *src_iter, struct customization_buffer *send_buf_st, size_t length, size_t *copy_length)
{
  *copy_length = 0;

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
  size_t i = 0;
  size_t remaining_length = recvmsg_ret;
  size_t loop_length = recvmsg_ret;
  u32 number_of_tags_removed = 0;
  u32 number_of_partial_tags_removed = 0;
  *copy_length = 0;

  total_bytes_from_server += recvmsg_ret;
  // trace_printk("L4.5: Total bytes from server to cust mod = %lu\n", total_bytes_from_server);

  // trace_printk("L4.5 Start Params: recvmsg=%lu, tag_bytes=%lu, extra_bytes=%lu, total_bytes=%lu\n", recvmsg_ret, tag_bytes_removed_last_round, extra_bytes_copied_from_last_send, total_bytes_from_server);
  // trace_print_hex_dump("Overkill: ", DUMP_PREFIX_ADDRESS, 16, 1, src_iter->iov->iov_base, recvmsg_ret, true);


  if (tag_bytes_removed_last_round > 0)
  {
    // we might have more tag bytes to strip before reaching for loop
    if (remaining_length >= (cust_tag_test_size - tag_bytes_removed_last_round))
    {
      // trace_print_hex_dump("Tag Removal Leftover: ", DUMP_PREFIX_ADDRESS, 16, 1, src_iter->iov->iov_base + src_iter->iov_offset, cust_tag_test_size - tag_bytes_removed_last_round, true);
      iov_iter_advance(src_iter, cust_tag_test_size - tag_bytes_removed_last_round);

      remaining_length -= (cust_tag_test_size - tag_bytes_removed_last_round);
      number_of_partial_tags_removed +=1;
      total_tags += 1;
      tag_bytes_removed_last_round = 0;
    }
    else
    {
      trace_printk("L4.5 ALERT: Hit edge case, just tag bytes left in packet\n");
      // just drop packet = TODO handle edge case
      return;
    }
  }

  if (extra_bytes_copied_from_last_send > 0)
  {
    // trace_printk("L4.5: extra = %lu, recvmsg_ret = %lu\n", extra_bytes_copied_from_last_send, recvmsg_ret);
    // we had bytes counting towards BYTE_POSIT from last send call
    if (BYTE_POSIT - extra_bytes_copied_from_last_send > recvmsg_ret)
    {
      // trace_printk("L4.5: not enough bytes to copy, bytes = %lu\n", recvmsg_ret);
      //copy entire buffer because still under BYTE_POSIT value
      copy_success = copy_from_iter_full(recv_buf_st->buf, recvmsg_ret, src_iter);
      if(copy_success == false)
      {
        trace_printk("L4.5 ALERT: Length not big enough, Failed to copy all bytes to cust buffer\n");
        // trace_print_hex_dump("iter buf: ", DUMP_PREFIX_ADDRESS, 16, 1, src_iter->iov->iov_base, 32, true);
        trace_print_cust_iov_params(src_iter);
        return;
      }
      // trace_printk("L4.5: Copied %lu bytes to cust buffer\n", recvmsg_ret);
      *copy_length += recvmsg_ret;
      extra_bytes_copied_from_last_send += recvmsg_ret;
      remaining_length = 0; // basically causes return call
    }
    else
    {
      // trace_printk("L4.5: enough bytes to copy, bytes = %lu\n", BYTE_POSIT - extra_bytes_copied_from_last_send);
      // first copy remaining bytes to reach BYTE_POSIT
      copy_success = copy_from_iter_full(recv_buf_st->buf, BYTE_POSIT - extra_bytes_copied_from_last_send, src_iter);
      if(copy_success == false)
      {
        // not all bytes were copied, so pick scenario 1 or 2 below
        trace_printk("L4.5 ALERT: Length check good, Failed to copy bytes to cust buffer\n");
        // trace_print_hex_dump("iter buf: ", DUMP_PREFIX_ADDRESS, 16, 1, src_iter->iov->iov_base, 32, true);
        trace_print_cust_iov_params(src_iter);
        // Scenario 1: keep cust loaded and allow normal msg to be sent
        *copy_length = 0;
        return;
      }
      // trace_printk("Copied %lu bytes to cust buffer\n", BYTE_POSIT - extra_bytes_copied_from_last_send);
      *copy_length += BYTE_POSIT - extra_bytes_copied_from_last_send;
      remaining_length -= *copy_length;
      extra_bytes_copied_from_last_send = 0;

      //now check if enough tag bytes to remove
      if (remaining_length >= cust_tag_test_size)
      {
        // trace_print_hex_dump("Tag Removal Extra: ", DUMP_PREFIX_ADDRESS, 16, 1, src_iter->iov->iov_base + src_iter->iov_offset, cust_tag_test_size, true);
        iov_iter_advance(src_iter, cust_tag_test_size);

        remaining_length -= cust_tag_test_size;
        number_of_tags_removed +=1;
        total_tags+=1;
      }
      else
      {
        tag_bytes_removed_last_round = remaining_length;
        remaining_length = 0;
        number_of_partial_tags_removed +=1;
      }
    }
  }

  loop_length = remaining_length;
  // trace_printk("L4.5: loop length = %lu\n", loop_length);
  // at this point we have 0 bytes inserted toward BYTE_POSIT tag positiion
  for (i = 0; i + BYTE_POSIT + cust_tag_test_size <= loop_length; i+=BYTE_POSIT + cust_tag_test_size)
  {
  	copy_success = copy_from_iter_full(recv_buf_st->buf + *copy_length, BYTE_POSIT, src_iter);
    if(copy_success == false)
    {
      // not all bytes were copied, so pick scenario 1 or 2 below
      trace_printk("L4.5 ALERT: For loop, Failed to copy bytes to cust buffer\n");
      // trace_print_hex_dump("iter buf: ", DUMP_PREFIX_ADDRESS, 16, 1, src_iter->iov->iov_base, 32, true);
      trace_print_cust_iov_params(src_iter);
      // Scenario 1: keep cust loaded and allow normal msg to be sent
      *copy_length = 0;
      return;
    }
    // trace_printk("L4.5: Copied %lu bytes to cust buffer\n", BYTE_POSIT);
    *copy_length += BYTE_POSIT;
    remaining_length -= BYTE_POSIT;

    // trace_print_hex_dump("Tag Removal Loop: ", DUMP_PREFIX_ADDRESS, 16, 1, src_iter->iov->iov_base + src_iter->iov_offset, cust_tag_test_size, true);
    // now advance past tag
    iov_iter_advance(src_iter, cust_tag_test_size);
    remaining_length -= cust_tag_test_size;
    number_of_tags_removed +=1;
    total_tags+=1;
  }


  if (remaining_length > 0)
  {
    // trace_printk("L4.5: remaining length = %lu\n", remaining_length);
    // if remaining length > BYTE_POSIT but less than BYTE_POSIT + cust_tag_test_size, then have a partial tag to deal with
    if (remaining_length > BYTE_POSIT)
    {
      copy_success = copy_from_iter_full(recv_buf_st->buf + *copy_length, BYTE_POSIT, src_iter);
      if(copy_success == false)
      {
        trace_printk("L4.5 ALERT: Failed to copy remaining bytes to cust buffer\n");
        // trace_print_hex_dump("iter buf: ", DUMP_PREFIX_ADDRESS, 16, 1, src_iter->iov->iov_base, 32, true);
        trace_print_cust_iov_params(src_iter);
        *copy_length = 0;
        return;
      }
      // trace_printk("L4.5: Copied %lu bytes to cust buffer\n", BYTE_POSIT);
      // trace_print_hex_dump("Tag Removal Remaining: ", DUMP_PREFIX_ADDRESS, 16, 1, src_iter->iov->iov_base + src_iter->iov_offset, remaining_length - BYTE_POSIT, true);
      tag_bytes_removed_last_round = remaining_length - BYTE_POSIT;
      extra_bytes_copied_from_last_send = 0;
      // no need to advance iter since we are done with this buffer
      *copy_length += BYTE_POSIT;
    }
    else
    {
      //copy over leftover bytes from loop
      copy_success = copy_from_iter_full(recv_buf_st->buf + *copy_length, remaining_length, src_iter);
      if(copy_success == false)
      {
        trace_printk("L4.5 ALERT: Failed to copy remaining bytes to cust buffer\n");
        // trace_print_hex_dump("iter buf: ", DUMP_PREFIX_ADDRESS, 16, 1, src_iter->iov->iov_base, 32, true);
        trace_print_cust_iov_params(src_iter);
        *copy_length = 0;
        return;
      }
      // trace_printk("L4.5: Copied %lu bytes to cust buffer\n", remaining_length);
      // trace_print_cust_iov_params(src_iter);
      extra_bytes_copied_from_last_send += remaining_length;
      *copy_length += remaining_length;
      tag_bytes_removed_last_round = 0;
    }
  }

  // trace_printk("L4.5: Number of tags removed = %u\n", number_of_tags_removed);
  // trace_printk("L4.5: Number of partial tags removed = %u\n", number_of_partial_tags_removed);
  // trace_printk("L4.5: Total tags fully removed = %lu\n", total_tags);

  app_bytes_from_server += *copy_length;
  // trace_printk("L4.5: Total app bytes from server to cust mod = %lu\n", app_bytes_from_server);
  // trace_printk("L4.5 End Params: tag_bytes=%lu, extra_bytes=%lu\n", tag_bytes_removed_last_round, extra_bytes_copied_from_last_send);
	return;
}



// The init function that calls the functions to register a Layer 4.5 customization
// @post Layer 4.5 customization registered
int __init sample_client_start(void)
{
	char application_name[16] = "curl";
  int result;

	client_cust = kmalloc(sizeof(struct customization_node), GFP_KERNEL);
	if(client_cust == NULL)
	{
		trace_printk("L4.5 ALERT: client kmalloc failed\n");
		return -1;
	}

  client_cust->target_flow.protocol = 6; // TCP
	memcpy(client_cust->target_flow.task_name, application_name, TASK_NAME_LEN);

	client_cust->target_flow.dest_port = 8080;
  client_cust->target_flow.source_port = 0;

  client_cust->target_flow.dest_ip = in_aton("10.0.0.20");
  client_cust->target_flow.source_ip = 0;

	client_cust->send_function = NULL;
	client_cust->recv_function = modify_buffer_recv;

	client_cust->cust_id = 78;
  client_cust->registration_time_struct.tv_sec = 0;
  client_cust->registration_time_struct.tv_nsec = 0;
	client_cust->retired_time_struct.tv_sec = 0;
  client_cust->retired_time_struct.tv_nsec = 0;

  client_cust->send_buffer_size = 0; //  normal buffer size
  client_cust->recv_buffer_size = 65536 * 2; //accept default buffer size

	result = register_customization(client_cust);

  if(result != 0)
  {
    trace_printk("L4.5 ALERT: Module failed registration, check debug logs\n");
    return -1;
  }

	trace_printk("L4.5: client module loaded, id=%d\n", client_cust->cust_id);

  return 0;
}


// Calls the functions to unregister customization node from use on sockets
// @post Layer 4.5 customization node unregistered
void __exit sample_client_end(void)
{
  //NOTE: this is only valid/safe place to call unregister (deadlock scenario)
  int ret = unregister_customization(client_cust);

  if(ret == 0){
    trace_printk("L4.5 ALERT: client module unload error\n");
  }
  else
  {
    trace_printk("L4.5: client module unloaded\n");
  }
  kfree(client_cust);
	return;
}


module_init(sample_client_start);
module_exit(sample_client_end);
MODULE_AUTHOR("Dan Lukaszewski");
MODULE_LICENSE("GPL");
