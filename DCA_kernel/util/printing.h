#ifndef _PRINTING_UTILS_H
#define _PRINTING_UTILS_H

#include "../common_structs.h"

#define MAX_BUFFER_SIZE 64


// Helper to dump message buffer into log to compare app msg buffer to the
// customization module buffer
// @param[I] message_buf Buffer to dump to log
// @param[I] len Number of bytes to dump up to the mazimum buffer size allotted
// @param[I] proto Protocol being used to help debugging
// @param[I] pid Process ID of current process owning the buffer
void log_dump_msg(void *message_buf, int len, u16 proto, pid_t pid);


// A copy of print_hex_dump but for trace_printk instead
// Provides an alternative to log_dump_msg
// @see print_hex_dump function for parameters
// example:  trace_print_hex_dump("layer4.5 udp msg msg before: ", DUMP_PREFIX_ADDRESS, 16, 1, msg->msg_iter.iov->iov_base, 192, true);
void trace_print_hex_dump(const char *prefix_str, int prefix_type, int rowsize, int groupsize, const void *buf, size_t len, bool ascii);


// Helper to log customization socket 4-tuple and customization status
// status = 0 when not customizing
// @param[I] cust_socket Customization socket in use
// @param[I] context Differentiate log messages based on where called from
void trace_print_cust_socket(struct customization_socket *cust_socket, char *context);


// Helper to dump message structure parameter to log for comparison
// @param[I] msg Message structure to dump (usually from app layer perspective)
void trace_print_msg_params(struct msghdr *msg);


void trace_print_iov_params(struct iov_iter *src_iter);


void trace_print_module_params(struct customization_node *cust_node);



#endif
