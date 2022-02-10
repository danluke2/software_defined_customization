#ifndef _TAP_H
#define _TAP_H


// Stores the old TCP function pointers and replaces them with custom functions
// socket calls tcp, which calls new_tcp, which calls ref_tcp
// @pre System TCP pointers point to the original tcp_prot functions
// @post System TCP pointers point to custom functions
void register_tcp_taps(void);


// Stores the old UDP function pointers and replaces them with custom functions.
// socket calls udp, which calls new_udp, which calls ref_udp
// @pre System UDP pointers point to the original udp_prot functions.
// @post System UDP pointers point to custom functions
void register_udp_taps(void);


// Restores the original TCP functions and frees the connection state.
// @post The tcp_prot functions point to the original TCP functions.
void unregister_tcp_taps(void);


// Restores the original UDP functions
// @post The udp_prot functions point to the original UDP functions.
void unregister_udp_taps(void);

#endif
