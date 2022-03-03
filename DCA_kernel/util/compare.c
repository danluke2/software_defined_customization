// @file utils/compare.c
// @breif The functions to perform comparison between a customization socket and
// node

#include "compare.h"


bool pid_task_compare(struct customization_node *node, struct customization_socket *socket)
{
  bool success = false;
  //if using a wildcard application name, then return true
  if(strncmp(node->target_flow.task_name_pid, "*", 1) == 0)
  {
    success = true;
  }
  else if(strncmp(node->target_flow.task_name_pid, socket->socket_flow.task_name_pid, TASK_NAME_LEN) == 0)
  {
    success = true;
  }
  return success;
}


bool tgid_task_compare(struct customization_node *node, struct customization_socket *socket)
{
  bool success = false;
  //if using a wildcard application name, then return true
  if(strncmp(node->target_flow.task_name_tgid, "*", 1) == 0)
  {
    success = true;
  }
  else if(strncmp(node->target_flow.task_name_tgid, socket->socket_flow.task_name_tgid, TASK_NAME_LEN) == 0)
  {
    success = true;
  }
  return success;
}


bool protocol_compare(struct customization_node *node, struct customization_socket *socket)
{
  bool success = false;
  //if protocol wildcard number specified, then return true
  if(node->target_flow.protocol == 256)
  {
    success = true;
  }
  else if(node->target_flow.protocol == socket->socket_flow.protocol)
  {
    success = true;
  }
  return success;
}


bool dest_compare(struct customization_node *node, struct customization_socket *socket)
{
  bool success = false;
  // if no dest port specified or ports match, then check IP addr
  if(node->target_flow.dest_port == 0 || node->target_flow.dest_port == socket->socket_flow.dest_port)
  {
    if(node->target_flow.dest_ip == 0 || node->target_flow.dest_ip == socket->socket_flow.dest_ip)
    {
      success = true;
    }
  }
  return success;
}


bool src_compare(struct customization_node *node, struct customization_socket *socket)
{
  bool success = false;

  if(node->target_flow.source_port == 0 || node->target_flow.source_port == socket->socket_flow.source_port)
  {
    if(node->target_flow.source_ip == 0 || node->target_flow.source_ip == socket->socket_flow.source_ip)
    {
      success = true;
    }
  }
  return success;
}




bool customization_compare(struct customization_node *node, struct customization_socket *socket)
{
  bool success = false;

	if(pid_task_compare(node, socket))
	{
    #ifdef DEBUG3
      trace_printk("Application pid name check passed, cust_id=%d\n", node->cust_id);
    #endif
    if(tgid_task_compare(node, socket))
  	{
      #ifdef DEBUG3
        trace_printk("Application tgid name check passed, cust_id=%d\n", node->cust_id);
      #endif
  		if(protocol_compare(node, socket))
  		{
        #ifdef DEBUG3
          trace_printk("Protocol check passed, cust_id=%d\n", node->cust_id);
  			#endif
        if(dest_compare(node, socket))
  			{
          #ifdef DEBUG3
            trace_printk("Dest check passed, cust_id=%d\n", node->cust_id);
  				#endif
          if(src_compare(node, socket))
  				{
            #ifdef DEBUG3
              trace_printk("Src check passed, cust_id=%d\n", node->cust_id);
  					#endif
            success = true;
  				}
  			}
  		}
    }
	}
return success;
}
