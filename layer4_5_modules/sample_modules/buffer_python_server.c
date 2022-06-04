// @file sample_python_server.c
// @brief A sample customization module to modify python3 send/recv calls

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/inet.h>
#include <linux/uio.h> // For iter structures

// ************** STANDARD PARAMS MUST GO HERE ****************
#include <common_structs.h>
#include <printing.h>
// ************** END STANDARD PARAMS ****************


extern int register_customization(struct customization_node *cust);

extern int unregister_customization(struct customization_node *cust);

// Kernel module parameters with default values
static char *destination_ip = "0.0.0.0";
module_param(destination_ip, charp, 0600);  //root only access to change
MODULE_PARM_DESC(destination_ip, "Dest IP to match");

static char *source_ip = "127.0.0.1";
module_param(source_ip, charp, 0600);
MODULE_PARM_DESC(source_ip, "Dest IP to match");

static unsigned int destination_port = 0;
module_param(destination_port, uint, 0600);
MODULE_PARM_DESC(destination_port, "DPORT to match");

static unsigned int source_port = 65432;
module_param(source_port, uint, 0600);
MODULE_PARM_DESC(source_port, "SPORT to match");

static unsigned int protocol = 256; // TCP or UDP
module_param(protocol, uint, 0600);
MODULE_PARM_DESC(protocol, "L4 protocol to match");


//test message for this plugin
char cust_test[12] = "testCustMod";


struct customization_node *python_cust;



// Function to customize the msg sent from the application to layer 4
// @param[I] send_buf_st Pointer to the send buffer structure
// @param[I] send_buf_st->src_iter Application message structure to copy from
// @param[I] send_buf_st->length Total size of the application message
// @param[X] send_buf_st->copy_length Number of bytes DCA needs to send to layer 4
// @pre send_buf_st->src_iter holds app message destined for Layer 4
// @post send_buf_st->src_buf holds customized message for DCA to send to Layer 4
void modify_buffer_send(struct customization_buffer *send_buf_st, struct customization_flow *socket_flow)
{
  bool copy_success;
  size_t cust_test_size = (size_t)sizeof(cust_test)-1;

	send_buf_st->copy_length = cust_test_size + send_buf_st->length;

  // send_buf could be realloc and change, thus update buf ptr and size if necessary
  // only necessary if you need to make the buffer larger than default size
	// send_buf_st->buf = krealloc(send_buf_st->buf, INSERT_NEW_LENGTH_HERE, GFP_KERNEL);
  // if(send_buf_st->buf==NULL)
  // {
  //   trace_printk("Realloc Failed\n");
  //   return;
  // }
  // send_buf_st->buf_size = INSERT_NEW_LENGTH_HERE;

	memcpy(send_buf_st->buf, cust_test, cust_test_size);
  //copy from full will revert iter back to normal if failure occurs
	copy_success = copy_from_iter_full(send_buf_st->buf+cust_test_size, send_buf_st->length, send_buf_st->src_iter);
  if(copy_success == false)
  {
    // not all bytes were copied, so pick scenario 1 or 2 below
    trace_printk("L4.5 ALERT: Failed to copy all bytes to cust buffer\n");
    // Scenario 1: keep cust loaded and allow normal msg to be sent
    send_buf_st->copy_length = 0;

    // Scenario 2: stop trying to customize this socket
    // kfree(send_buf_st->buf);
    // send_buf_st->buf = NULL;
    // copy_length = 0;
  }
	return;
}


// Function to customize the msg recieved from L4 prior to delivery to application
// @param[I] recv_buf_st Pointer to the recv buffer structure
// @param[I] recv_buf_st->src_iter Application message structure to copy from
// @param[I] recv_buf_st->length Max bytes application can receive;
// @param[I] recv_buf_st->recv_return Total size of message in src_iter
// @param[X] recv_buf_st->copy_length Number of bytes DCA needs to send to application
// @pre recv_buf_st->src_iter holds app message destined for application
// @post recv_buf_st->recv_buf holds customized message for DCA to send to app instead

// NOTE: copy_length must be <= length!!!!!!!!!

void modify_buffer_recv(struct customization_buffer *recv_buf_st, struct customization_flow *socket_flow)
{
  bool copy_success;
  size_t cust_test_size = (size_t)sizeof(cust_test)-1;
  recv_buf_st->no_cust = false;
  recv_buf_st->skip_cust = false;
  recv_buf_st->buffered_bytes = 0;


  trace_printk("L4.5 module: recvmsg_ret = %d, msg len = %lu\n", recv_buf_st->recv_return, recv_buf_st->length);

  
  // we aren't buffering, so if recvmsg returned error, just pass back to app
  if(recv_buf_st->recv_return < 0)
  {
    return;
  }

  


  //NOTE: when buffering allowed, recv_return can be 0
  if(recv_buf_st->recv_return == 0)
  {
    recv_buf_st->copy_length = 0;
  }
  else
  {
    trace_print_hex_dump("Temp Buffer Message: ", DUMP_PREFIX_ADDRESS, 16, 1, recv_buf_st->temp_buf, recv_buf_st->recv_return, true);
    
    if(recv_buf_st->recv_return - cust_test_size <=0)
    {
      if(recv_buf_st->recv_return <= recv_buf_st->length)
      {
        recv_buf_st->no_cust = true;
      }
      else
      {
        // need to buffer some data b/c app won't accept
        // for now we just drop the extra data
        recv_buf_st->copy_length = recv_buf_st->length;
        copy_success = copy_from_iter_full(recv_buf_st->buf, recv_buf_st->copy_length, &recv_buf_st->kmsg.msg_iter);
      }
    }
    else
    {
      recv_buf_st->copy_length = recv_buf_st->recv_return - cust_test_size;

      if(recv_buf_st->copy_length > recv_buf_st->length)
      {
        //just copy what we can and drop the rest
        recv_buf_st->copy_length = recv_buf_st->length;
      }

      trace_printk("L4.5 module: memcpy kmsg iter buf\n");
      memcpy(recv_buf_st->buf, recv_buf_st->temp_buf + cust_test_size, recv_buf_st->copy_length);

      // trace_printk("L4.5 module: advancing kmsg iter buf\n");
      // //adjust iter offset to start of actual message, then copy
      // iov_iter_advance(&recv_buf_st->kmsg.msg_iter, cust_test_size);
      //
      // trace_printk("L4.5 module: copying %lu bytes to recv buffer\n", recv_buf_st->copy_length);
      //
      // copy_success = copy_from_iter_full(recv_buf_st->buf, recv_buf_st->copy_length, &recv_buf_st->kmsg.msg_iter);
      // if(copy_success == false)
      // {
      //   // not all bytes were copied, so pick scenario 1 or 2 below
      //   trace_printk("L4.5 ALERT: Failed to copy all bytes to cust buffer\n");
      //   // Scenario 1: keep cust loaded and allow normal msg to be sent
      //   recv_buf_st->no_cust = true;
      //
      //   // Scenario 2: stop trying to customize this socket
      //   // recv_buf_st->skip_cust = true;
      // }
    }
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
	char thread_name[16] = "python3"; //16 b/c size of kernel char array
  char application_name[16] = "python3"; //16 b/c size of kernel char array
  int result;

	python_cust = kmalloc(sizeof(struct customization_node), GFP_KERNEL);
	if(python_cust == NULL)
	{
		trace_printk("L4.5 ALERT: server kmalloc failed\n");
		return -1;
	}

  // python_cust->target_flow.protocol = 17; //UDP
  // python_cust->protocol = 6; //TCP
  python_cust->target_flow.protocol = (u16) protocol; // TCP and UDP
  memcpy(python_cust->target_flow.task_name_pid, thread_name, TASK_NAME_LEN);
  memcpy(python_cust->target_flow.task_name_tgid, application_name, TASK_NAME_LEN);

  // Server: source IP or port set b/c bind is called at setup
	python_cust->target_flow.dest_port = (u16) destination_port; // set if you know client port
  python_cust->target_flow.source_port = (u16) source_port;

  //IP is a __be32 value
  python_cust->target_flow.dest_ip = in_aton(destination_ip);
  python_cust->target_flow.source_ip = in_aton(source_ip);

  // These functions must be defined and will be used to modify the
  // buffer on a send/receive call
  // Function can be set to NULL if not modifying buffer contents
	python_cust->send_function = NULL;
	python_cust->recv_function = modify_buffer_recv;

  // Cust ID normally set by NCO, uniqueness required
	python_cust->cust_id = 24;
  python_cust->registration_time_struct.tv_sec = 0;
  python_cust->registration_time_struct.tv_nsec = 0;
  python_cust->retired_time_struct.tv_sec = 0;
  python_cust->retired_time_struct.tv_nsec = 0;

  python_cust->send_buffer_size = 0; //  normal buffer size
  python_cust->recv_buffer_size = 0; //  normal buffer size
  // temp_buffer_size <= recv_buffer_size
  python_cust->temp_buffer_size = 0; //  normal buffer size

	result = register_customization(python_cust);

  if(result != 0)
  {
    trace_printk("L4.5 ALERT: Module failed registration, check debug logs\n");
    return -1;
  }

	trace_printk("L4.5: server module loaded, id=%d\n", python_cust->cust_id);

  return 0;
}

// Calls the functions to unregister customization node from use on sockets
// @post Layer 4.5 customization node unregistered
void __exit sample_server_end(void)
{
  int ret = unregister_customization(python_cust);

  if(ret == 0)
  {
    trace_printk("L4.5 ALERT: server module unload error\n");
  }
  else
  {
    trace_printk("L4.5: server module unloaded\n");
  }
  kfree(python_cust);
	return;
}

module_init(sample_server_start);
module_exit(sample_server_end);
MODULE_AUTHOR("Dan Lukaszewski");
MODULE_LICENSE("GPL");
