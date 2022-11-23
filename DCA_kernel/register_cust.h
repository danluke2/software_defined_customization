#ifndef _CUSTOMIZATION_REGISTRATION_H
#define _CUSTOMIZATION_REGISTRATION_H


#include "common_structs.h"


// initializes the customization list pointers
// @see list.h for list functions
void init_customization_list(void);


// Removes each entry in the list while holding lock to prevent access from other functions
// @see list.h for list functions
void free_customization_list(void);


// Removes each entry in the deprecated list while holding lock to prevent access from other functions
// @see list.h for list functions
void free_deprecated_customization_list(void);


// Removes each entry in the revoked list while holding lock to prevent access from other functions
// @see list.h for list functions
void free_revoked_customization_list(void);


// Finds a customization present in the linked list that matches the cust_id
// Used by netlink helper to issue challenge_response call to module
// @param[I] cust_id The module ID of the customization installed
// @return The customization node that applies to the cust_id
struct customization_node *get_cust_by_id(u16 cust_id);


// Finds up to MAX_CUST_ATTACH customizations present in the linked list that apply to socket
// Servers call this on first message received
// Clients call on first message sent
// @param[I] cust_socket The desired socket to match customization to
// @param[I] nodes[] Holds MAX_CUST_ATTACH nodes that match the socket; assumed to hold NULL ptrs
// @see util/compare.c: compare functions
// @return The sorted customization node array that applies to the input socket
size_t get_customizations(struct customization_socket *cust_socket, struct customization_node *nodes[MAX_CUST_ATTACH]);


// Registers a new protocol customization module available
// Layer 4.5 makes a copy of module_cust and stores a local id in cust
// @param[I] module_cust Pre-filled customization node
// @param[I] applyNow Request customization to apply to all applicable sockets, not just new sockets
// @return int for success/failure
// @post Customization node with all variables set added to customization list
int register_customization(struct customization_node *module_cust, u16 applyNow);


// Removes the registered protocol customization module from list and any
// active sockets using it
// @param[I] module_cust customization module to match copy in list
// @see customization_socket.c: remove_customization_from_each_socket
// @return int for success/failure
int unregister_customization(struct customization_node *module_cust);


// Called by DCA to stop new sockets from using the registered module
// but DCA only has the module ID, not the struct pointer
// @param[I] cust_id Registered customization nodes NCO generated id
// @return int for success/failure
int remove_customization_from_active_list(u16 cust_id);


// NETLINK support function reporting to DCA
// @param[I] message The allocated message buffer to hold standard report
// @param[I] length The size of the message buffer
void netlink_cust_report(char *message, size_t *length);


// NETLINK support function reporting challenge_response reply to DCA
// @param[I] message The allocated message buffer to hold standard report
// @param[I] length The size of the message buffer
// @param[I] request The message sent by NCO to DCA [requires parsing]
void netlink_challenge_cust(char *message, size_t *length, char *request);


// NETLINK support function reporting deprecate request reply to DCA
// @param[I] message The allocated message buffer to hold standard report
// @param[I] length The size of the message buffer
// @param[I] request The message sent by NCO to DCA [requires parsing]
void netlink_deprecate_cust(char *message, size_t *length, char *request);


// NETLINK support function reporting active mode toggle request reply to DCA
// @param[I] message The allocated message buffer to hold standard report
// @param[I] length The size of the message buffer
// @param[I] request The message sent by NCO to DCA [requires parsing]
void netlink_toggle_cust(char *message, size_t *length, char *request);


// NETLINK support function reporting priority set request reply to DCA
// @param[I] message The allocated message buffer to hold standard report
// @param[I] length The size of the message buffer
// @param[I] request The message sent by NCO to DCA [requires parsing]
void netlink_set_cust_priority(char *message, size_t *length, char *request);

#endif
