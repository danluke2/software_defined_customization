// @file util/helpers.c
// @brief The helper functions available to modules

#include "helpers.h"
#include <linux/kernel.h>

// export for cust module use
void
trace_print_hex_dump(const char* prefix_str,
                     int prefix_type,
                     int rowsize,
                     int groupsize,
                     const void* buf,
                     size_t len,
                     bool ascii);
EXPORT_SYMBOL_GPL(trace_print_hex_dump);

void
set_module_struct_flags(struct customization_buffer* buf, bool flag_set);
EXPORT_SYMBOL_GPL(set_module_struct_flags);

void
set_module_struct_flags(struct customization_buffer* buf, bool flag_set)
{
  buf->try_next = flag_set;
  buf->set_cust_to_skip = flag_set;
  buf->try_next = flag_set;
};

// Printing helpers
void
log_dump_msg(void* message_buf, int len, u16 proto, pid_t pid)
{
  char temp[MAX_BUFFER_SIZE];
  unsigned char* bufchar = message_buf;
  int x;

  if (len > MAX_BUFFER_SIZE) {
    len = MAX_BUFFER_SIZE;
  }

  for (x = 0; x < len; x++) {
    temp[x] = *(bufchar + x);
  }
  temp[len] = '\0';
  trace_printk("L4.5: %u buffer dump, pid %d: %s\n", proto, pid, temp);
}

void
trace_print_hex_dump(const char* prefix_str,
                     int prefix_type,
                     int rowsize,
                     int groupsize,
                     const void* buf,
                     size_t len,
                     bool ascii)
{
  const u8* ptr = buf;
  int i, linelen, remaining = len;
  unsigned char linebuf[32 * 3 + 2 + 32 + 1];

  if (rowsize != 16 && rowsize != 32)
    rowsize = 16;

  for (i = 0; i < len; i += rowsize) {
    linelen = min(remaining, rowsize);
    remaining -= rowsize;

    hex_dump_to_buffer(
      ptr + i, linelen, rowsize, groupsize, linebuf, sizeof(linebuf), ascii);

    switch (prefix_type) {
      case DUMP_PREFIX_ADDRESS:
        trace_printk("%s%p: %s\n", prefix_str, ptr + i, linebuf);
        break;
      case DUMP_PREFIX_OFFSET:
        trace_printk("%s%.8x: %s\n", prefix_str, i, linebuf);
        break;
      default:
        trace_printk("%s%s\n", prefix_str, linebuf);
        break;
    }
  }
}

void
trace_print_cust_socket(struct customization_socket* cust_socket, char* context)
{
  trace_printk(
    "L4.5 %s: dest_ip:port=%pI4:%u, source_ip:port=%pI4:%u, cust=%d\n",
    context,
    &cust_socket->socket_flow.dest_ip,
    cust_socket->socket_flow.dest_port,
    &cust_socket->socket_flow.source_ip,
    cust_socket->socket_flow.source_port,
    cust_socket->customize_or_skip);
}

void
trace_print_msg_params(struct msghdr* msg)
{
  trace_printk("msg control len = %lu\n",
               msg->msg_controllen); // control buffer length
  trace_printk("msg flags as integer= %d\n", msg->msg_flags);
  trace_printk("msg iter type = %u\n", iov_iter_type(&msg->msg_iter));
  // trace_printk("msg iov len = %lu\n", msg->msg_iter.iov->iov_len);
  trace_printk("msg iov offset = %lu\n", msg->msg_iter.iov_offset);
  trace_printk(
    "Total amount of data pointed to by the iovec array (count) = %lu\n",
    msg->msg_iter.count);
  trace_printk("Number of iovec structures (nr_segs) = %lu\n",
               msg->msg_iter.nr_segs);
}

void
trace_print_iov_params(struct iov_iter* src_iter)
{
  // trace_printk("msg iov len = %lu; offset = %lu\n", src_iter->iov->iov_len,
  // src_iter->iov_offset);
  trace_printk(
    "Total amount of data pointed to by the iovec array (count) = %lu\n",
    src_iter->count);
  trace_printk("Number of iovec structures (nr_segs) = %lu\n",
               src_iter->nr_segs);
}

void
trace_print_module_params(struct customization_node* cust_node)
{
  trace_printk("Node protocol = %u\n", cust_node->target_flow.protocol);
  trace_printk("Node pid task = %s\n", cust_node->target_flow.task_name_pid);
  trace_printk("Node tgid task = %s\n", cust_node->target_flow.task_name_tgid);
  trace_printk("Node id = %u\n", cust_node->cust_id);
  trace_printk("Node Activated = %u\n", *cust_node->active_mode);
  trace_printk("Node Priority = %u\n", *cust_node->cust_priority);
  trace_printk("Node dest port = %u\n", cust_node->target_flow.dest_port);
  trace_printk("Node source port = %u\n", cust_node->target_flow.source_port);
  trace_printk("Node dest_ip = %pI4\n", &cust_node->target_flow.dest_ip);
  trace_printk("Node src_ip = %pI4\n", &cust_node->target_flow.source_ip);
}
