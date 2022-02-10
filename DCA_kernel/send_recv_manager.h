#ifndef _SEND_RECV_MANAGEMENT_H
#define _SEND_RECV_MANAGEMENT_H


#include "common_structs.h"


// Handles sending application message to any customization modules that are
// linked to the socket prior to delivery to Layer4 method
// @param[I] cust_sock customized sock struct used to customize msg
// @param[I] sk sock struct sent to L4 sendmsg
// @param[I] msg holds application message iovec
// @param[I] size total size of app message
// @param[I] sendmsg function pointer to the applicable L4 sendmsg function
// @pre msg holds app message destined for Layer 4
// @post msg is customized and transferred to kvec and sent to L4
// @return number of bytes app expected to send or error
int dca_sendmsg(struct customization_socket *cust_sock, struct sock *sk,
							  struct msghdr *msg, size_t size,
								int (*sendmsg)(struct sock*, struct msghdr*, size_t));



// Handles sending application message to any customization modules that are
// linked to the socket prior to delivery to application
// @param[I] cust_sock customized sock struct used to customize msg
// @param[I] sk sock struct sent to L4 sendmsg
// @param[I] msg holds application message iovec
// @param[I] len total bytes app can receive
// @param[I] recv_ret total bytes L4 recv call received and put in msg
// @pre msg holds app message from Layer 4 recv call
// @post msg is customized and overwritten if necessary prior to delivery to app
// @return number of bytes in msg for app or error
int dca_recvmsg(struct customization_socket *cust_sock, struct sock *sk,
								struct msghdr *msg, size_t len, size_t recv_ret);


#endif
