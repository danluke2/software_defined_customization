#ifndef _CUSTOMIZATION_REGISTRATION_H
#define _CUSTOMIZATION_REGISTRATION_H


#include "common_structs.h"


// initializes the customization list pointers
// @see list.h for list functions
void init_customization_list(void);


// Removes each entry in the list while holding lock to prevent access from
// other functions
// @see list.h for list functions
void free_customization_list(void);


// Removes each entry in the retired list while holding lock to prevent access from
// other functions
// @see list.h for list functions
void free_retired_customization_list(void);


// Finds a customization present in the linked list that matches the cust_id
// Used by netlink helper to issue challenge_response call to module
// @param[I] cust_id The module ID of the customization installed
// @return The customization node that applies to the cust_id
struct customization_node *get_cust_by_id(u16 cust_id);


// Finds a customization present in the linked list that applies to socket
// Servers call this on first message received
// Clients call on first message sent
// @param[I] cust_socket The desired socket to match customization to
// @see util/compare.c: compare functions
// @return The customization node that applies to the input socket
struct customization_node *get_customization(struct customization_socket *cust_socket);


// Registers a new protocol customization module available
// Layer 4.5 makes a copy of module_cust and stores a local id in cust
// @param[I] module_cust Pre-filled customization node
// @return int for success/failure
// @post Customization node with all variables set added to customization list
int register_customization(struct customization_node *module_cust);


// Removes the registered protocol customization module from list and any
// active sockets using it
// @param[I] cust_id Registered customization nodes server generated id
// @param[I] local_id Registered customization nodes local id
// @see customization_socket.c: remove_customization_from_each_socket
// @return int for success/failure
int unregister_customization(struct customization_node *module_cust);


// NETLINK support function reporting to customization assistant
// @param[I] message The allocated message buffer to hold standard report
// @param[I] length The size of the message buffer
void netlink_cust_report(char *message, size_t *length);


// NETLINK support function reporting cahllenge_response reply to customization assistant
// @param[I] message The allocated message buffer to hold standard report
// @param[I] length The size of the message buffer
// @param[I] request The message sent by NCO to DCA for challenging [requires parsing]
void netlink_challenge_cust(char *message, size_t *length, char *request);

#endif