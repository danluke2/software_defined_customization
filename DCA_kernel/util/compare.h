#ifndef _COMPARISON_UTILS_H
#define _COMPARISON_UTILS_H

#include <common_structs.h>


// Compares a customization nodes application/task name to the given socket
// based on the pid (i.e., thread)
// @param[I] node Customization node registered on host
// @param[I] socket Intercepted socket to compare against
// @return bool for pass/fail of compare check
bool pid_task_compare(struct customization_node *node, struct customization_socket *socket);


// Compares a customization nodes application/task name to the given socket
// based on the thread group (i.e. process)
// @param[I] node Customization node registered on host
// @param[I] socket Intercepted socket to compare against
// @return bool for pass/fail of compare check
bool tgid_task_compare(struct customization_node *node, struct customization_socket *socket);


// Compares a customization nodes protocol value to the given socket
// @param[I] node Customization node registered on host
// @param[I] socket Intercepted socket to compare against
// @return bool for pass/fail of compare check
bool protocol_compare(struct customization_node *node, struct customization_socket *socket);


// Compares a customization nodes destination IP and port to the given socket
// @param[I] node Customization node registered on host
// @param[I] socket Intercepted socket to compare against
// @return bool for pass/fail of compare check
bool dest_compare(struct customization_node *node, struct customization_socket *socket);


// Compares a customization nodes source IP and port to the given socket
// @param[I] node Customization node registered on host
// @param[I] socket Intercepted socket to compare against
// @return bool for pass/fail of compare check
bool src_compare(struct customization_node *node, struct customization_socket *socket);



// Compares a customization node parameters to the given socket
// @param[I] node Customization node registered on host
// @param[I] socket Intercepted socket to compare against
// @return bool for pass/fail of compare check
bool customization_compare(struct customization_node *node, struct customization_socket *socket);



// Comparison function used with sort to compare cust node priority
// @param[I] lhs pointer to left element of customization array
// @param[I] rhs pointer to right element of customization array
// @return Integer signaling comparison result
int priority_compare(const void *lhs, const void *rhs);

#endif
