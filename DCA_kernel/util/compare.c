// @file utils/compare.c
// @breif The functions to perform comparison between a customization socket and
// node

#include "compare.h"


bool task_compare(struct customization_node *node, struct customization_socket *socket)
{
  bool success = false;
  //if using a wildcard application name, then return true
  if(strncmp(node->target_flow.task_name, "*", 1) == 0)
  {
    success = true;
  }
  else if(strncmp(node->target_flow.task_name, socket->socket_flow.task_name, TASK_NAME_LEN) == 0)
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
