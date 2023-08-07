#ifndef _CUSTOMIZATION_DEFINES_H
#define _CUSTOMIZATION_DEFINES_H

#define MAX_CUST_ATTACH 5
#define TASK_NAME_LEN 16 // defined in sched.h as TASK_COMM_LEN 16
#define SEND_BUF_SIZE 65536
#define RECV_BUF_SIZE 65536

// for functions that need to know direction of messages
#define SEND 0
#define RECV 1

// for modules challenge_response support
#define AES_BLOCK_SIZE 16
#define SYMMETRIC_KEY_LENGTH 32
#define HEX_KEY_LENGTH 64
#define IV_LENGTH 16
#define HEX_IV_LENGTH 32
#define CHALLENGE_LENGTH 16
#define HEX_CHALLENGE_LENGTH 32
#define RESPONSE_LENGTH 32
#define HEX_RESPONSE_LENGTH 64

#endif