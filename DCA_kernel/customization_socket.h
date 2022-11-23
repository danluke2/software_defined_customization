#ifndef _CUSTOMIZATION_SOCKET_H
#define _CUSTOMIZATION_SOCKET_H

#include <net/sock.h>

#include "common_structs.h"


// Creates the customization socket table to hold processed sockets
// @see hashtable.h for hash_init
// @post initialized table and memory tracking
void init_socket_tables(void);


// Creates a customizable socket and adds it to the hash table.
// Determines if customization will apply to socket based on registered custs
// @param[I] pid The desired Process ID
// @param[I] sk A pointer to socket struct for the connection
// @param[I] msg A pointer to msghdr for setting four tuple (TCP sets as NULL)
// @param[I] name The application name of the intercepted process (e.g., python3)
// @param[I] dir The direction of the packet (SEND or RECV)
// @return a pointer to the new customizable socket
struct customization_socket *create_cust_socket(struct task_struct *task, struct sock *sk, struct msghdr *msg);


// Checks if the customizable socket now matches any new cust modules
// @param[I] cust_socket The customizable socket
// @param[I] task current task pointer
// @param[I] sk A pointer to socket struct for the connection
void update_cust_status(struct customization_socket *cust_socket, struct task_struct *task, struct sock *sk);



// Finds a customizable socket in the hash table
// @param[I] pid The desired connection's Process ID.
// @param[I] sk A pointer to socket struct for the connection.
// @return The customizable socket
struct customization_socket *get_cust_socket(struct task_struct *task, struct sock *sk);



// Sets customization update check to true for each socket in table not customized
// Called when new cust module registered with applyNow flag set
void set_update_cust_check(void);


// Look at each socket and see if the stored customization matches cust_id
// @param[I] cust The registered customiztion to search for
// @post The customization is no longer active on any socket and safe to unload
void remove_customization_from_each_socket(struct customization_node *cust);


// Look at each socket and see if the stored customization matches cust_id
// Called when a customization module gets a new priority level
// @param[I] cust The registered customiztion to search for
// @post The matching socket is sorted again
// void sort_each_socket_with_matching_cust(struct customization_node *cust);

// Look at each socket and see if the stored customization matches cust_id
// Called when a customization module gets a new priority level
// @param[I] cust The registered customiztion to search for
// @post The matching socket is marked for sorting on next use
void sort_mark_each_socket_with_matching_cust(struct customization_node *cust);


// Sorts the customization modules attached to the cust socket
// @param[I] cust The registered customiztion to search for
// @post The matching socket is sorted
void sort_attached_cust(struct customization_socket *cust_sock);


// Deletes a single cust socket
// @param[I] pid The desired Process ID.
// @param[I] sk A pointer to sock struct for the connection.
// @return 1 if a cust socket was found and deleted, 0 otherwise
int delete_cust_socket(struct task_struct *task, struct sock *sk);


// Deletes all of the connections in the hash table.
void delete_all_cust_socket(void);

#ifdef DEBUG
// A debug tool to print all the stored connection states in the hash
void print_all_cust_socket(void);
#endif


#endif
