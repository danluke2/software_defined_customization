// @file core_modules/NCO_test.c
// @brief Test module to verify challenge-response works

#include <linux/inet.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uio.h> // For iter structures
#include <linux/version.h>

// crypto stuff
#include <crypto/internal/skcipher.h>
#include <linux/crypto.h>
#include <linux/random.h>
#include <linux/scatterlist.h>

// NCO copies common_structs to each host directory (one level above modules dir)
#include "../common_structs.h"


extern int register_customization(struct customization_node *cust, u16 applyNow);

extern int unregister_customization(struct customization_node *cust);


// test message for this plugin
char cust_test[12] = "testCustMod";


struct customization_node *python_cust;


/* tie all data structures together */
struct skcipher_def
{
    struct scatterlist sg;
    struct crypto_skcipher *tfm;
    struct skcipher_request *req;
    struct crypto_wait wait;
};


// established during init function from hex key added during make process
u8 byte_key[SYMMETRIC_KEY_LENGTH] = "";



// NCO VARIABLES GO HERE
u16 module_id = 1;
char hex_key[HEX_KEY_LENGTH] = "";
u16 bypass = 0;
u16 priority = 0;
u16 applyNow = 0;


// END NCO VARIABLES




void modify_buffer_send(struct customization_buffer *send_buf_st, struct customization_flow *socket_flow)
{
    // passive monitoring allowed here, but active must pass bypass_mode check

    if (*python_cust->bypass_mode)
    {
        send_buf_st->try_next = true;
        return;
    }


    send_buf_st->no_cust = true;
    return;
}



void modify_buffer_recv(struct customization_buffer *recv_buf_st, struct customization_flow *socket_flow)
{
    // passive monitoring allowed here, but active must pass bypass_mode check

    if (*python_cust->bypass_mode)
    {
        recv_buf_st->try_next = true;
        return;
    }


    recv_buf_st->no_cust = true;
    return;
}



/* Perform desired cipher operation */
static unsigned int test_skcipher_encdec(struct skcipher_def *sk, int enc)
{
    int rc;

    if (enc)
        rc = crypto_wait_req(crypto_skcipher_encrypt(sk->req), &sk->wait);
    else
        rc = crypto_wait_req(crypto_skcipher_decrypt(sk->req), &sk->wait);

    if (rc)
        trace_printk("skcipher encrypt returned with result %d\n", rc);

    return rc;
}


// hex is char array holding hex string to convert to bytes (2x larger than bytes)
// bytes is char array to hold hex string as bytes
// length is the size of the bytes array
void hex_to_bytes(char *hex, u8 *bytes, size_t length)
{
    size_t i;
    size_t j;

    for (i = 0, j = 0; j < length; i += 2, j++)
    {
        bytes[j] = (hex[i] % 32 + 9) % 25 * 16 + (hex[i + 1] % 32 + 9) % 25;
    }
}


// bytes is char array to be converted to hex string
// hex is char array twice as large as bytes to hold hex string of bytes
// length is the size of the bytes array
void bytes_to_hex(u8 *bytes, char *hex, size_t length)
{
    size_t j;

    for (j = 0; j < length; j++)
    {
        hex[2 * j] = (bytes[j] >> 4) + 48;
        hex[2 * j + 1] = (bytes[j] & 15) + 48;
        if (hex[2 * j] > 57)
            hex[2 * j] += 7;
        if (hex[2 * j + 1] > 57)
            hex[2 * j + 1] += 7;
    }
}


// Builder will write in the key that is used for the crypto, and module_ID
// thus module has no knowledge of it before build, and NCO has it stored for reference
// takes in a challenge string, decodes it with module key, adds to it, encodes and returns
void challenge_response(char *message, char *iv, char *challenge)
{
    struct skcipher_def sk;
    struct crypto_skcipher *skcipher = NULL;
    struct skcipher_request *req = NULL;
    u8 scratchpad[CHALLENGE_LENGTH] = "";
    u8 ivdata[IV_LENGTH] = "";
    int ret = -EFAULT;
    char response_id[16] = "";


    // response will just be module_id as a 6 char hex string added to end of challenge
    sprintf(response_id, "%06x", module_id);
    memcpy(response_id + 6, "0505050505", 10); // required padding
    // print_hex_dump(KERN_DEBUG, "L4.5 response_id: ", DUMP_PREFIX_ADDRESS, 16, 1, response_id, 16, true);

    // print_hex_dump(KERN_DEBUG, "L4.5 hex-key: ", DUMP_PREFIX_ADDRESS, 16, 1, hex_key, 64, true);
    // print_hex_dump(KERN_DEBUG, "L4.5 bytes-key: ", DUMP_PREFIX_ADDRESS, 16, 1, byte_key, 32, true);

    // setup crypto algo, allocate space, and set callback
    skcipher = crypto_alloc_skcipher("cbc-aes-aesni", 0, 0);
    if (IS_ERR(skcipher))
    {
        trace_printk("L4.5 AELRT: could not allocate skcipher handle\n");
        memcpy(message, "ERROR", 5);
        return;
    }

    req = skcipher_request_alloc(skcipher, GFP_KERNEL);
    if (!req)
    {
        trace_printk("L4.5 AELRT: could not allocate skcipher request\n");
        ret = -ENOMEM;
        memcpy(message, "ERROR", 5);
        goto out;
    }

    skcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG, crypto_req_done, &sk.wait);


    // uses global key inserted when module is built
    if (crypto_skcipher_setkey(skcipher, byte_key, SYMMETRIC_KEY_LENGTH))
    {
        trace_printk("L4.5 AELRT: key could not be set\n");
        ret = -EAGAIN;
        memcpy(message, "ERROR", 5);
        goto out;
    }

    // set IV to bytes from PCC
    hex_to_bytes(iv, ivdata, IV_LENGTH);

    // print_hex_dump(KERN_DEBUG, "L4.5 iv: ", DUMP_PREFIX_ADDRESS, 16, 1, ivdata, 16, true);

    // set scratchpad to bytes from PCC
    hex_to_bytes(challenge, scratchpad, CHALLENGE_LENGTH);

    // print_hex_dump(KERN_DEBUG, "L4.5 challenge: ", DUMP_PREFIX_ADDRESS, 16, 1, scratchpad, 16, true);


    sk.tfm = skcipher;
    sk.req = req;

    /* We encrypt/decrypt one block */
    /**
     * sg_init_one - Initialize a single entry sg list
     * @sg:		 SG entry
     * @buf:	 Virtual address for IO
     * @buflen:	 IO length
     *
     **/
    sg_init_one(&sk.sg, scratchpad, AES_BLOCK_SIZE);
    /**
     * skcipher_request_set_crypt() - set data buffers
     * @req: request handle
     * @src: source scatter / gather list
     * @dst: destination scatter / gather list
     * @cryptlen: number of bytes to process from @src
     * @iv: IV for the cipher operation which must comply with the IV size defined
     *      by crypto_skcipher_ivsize
     *
     * This function allows setting of the source data and destination data
     * scatter / gather lists.
     *
     * For encryption, the source is treated as the plaintext and the
     * destination is the ciphertext. For a decryption operation, the use is
     * reversed - the source is the ciphertext and the destination is the plaintext.
     */
    skcipher_request_set_crypt(req, &sk.sg, &sk.sg, AES_BLOCK_SIZE, ivdata);
    crypto_init_wait(&sk.wait);

    /* decrypt data */
    ret = test_skcipher_encdec(&sk, 0);
    if (ret)
    {
        memcpy(message, "ERROR", 5);
        goto out;
    }


    trace_printk("L4.5: Decryption triggered successfully\n");
    // print_hex_dump(KERN_DEBUG, "L4.5 decrypted data: ", DUMP_PREFIX_ADDRESS, 16, 1, scratchpad, 16, true);

    // now ivdata holds a different IV value that we use for now as encrypt IV
    // print_hex_dump(KERN_DEBUG, "L4.5 encrypt iv: ", DUMP_PREFIX_ADDRESS, 16, 1, ivdata, 16, true);
    // copy iv as hex string to response message
    bytes_to_hex(ivdata, message, IV_LENGTH);

    // scratchpad first 8 bytes is challenge bytes, append response_id for encryption
    hex_to_bytes(response_id, scratchpad + 8, 8); // 8 = half of response size

    // print_hex_dump(KERN_DEBUG, "L4.5 decrypted data+response+pad: ", DUMP_PREFIX_ADDRESS, 16, 1, scratchpad, 16,
    // true);

    // now we need to update scratchpad with padding values
    // hex_to_bytes(padding_hex, scratchpad+8+3, 5); // 5 = half of pad size
    // print_hex_dump(KERN_DEBUG, "L4.5 decrypted data+response+padding: ", DUMP_PREFIX_ADDRESS, 16, 1, scratchpad, 16,
    // true);

    // i don't know if these steps are required again
    sg_init_one(&sk.sg, scratchpad, AES_BLOCK_SIZE);
    skcipher_request_set_crypt(req, &sk.sg, &sk.sg, AES_BLOCK_SIZE, ivdata);
    crypto_init_wait(&sk.wait);

    /* encrypt data */
    ret = test_skcipher_encdec(&sk, 1);
    if (ret)
    {
        memcpy(message + IV_LENGTH, "ERROR", 5);
        goto out;
    }


    trace_printk("L4.5: Encryption triggered successfully\n");
    // print_hex_dump(KERN_DEBUG, "L4.5 encrypted data: ", DUMP_PREFIX_ADDRESS, 16, 1, scratchpad, 16, true);

    // start at message offset since we already put iv hex string in it
    bytes_to_hex(scratchpad, message + HEX_IV_LENGTH, CHALLENGE_LENGTH);


out:
    if (skcipher)
        crypto_free_skcipher(skcipher);
    if (req)
        skcipher_request_free(req);
    return;
}




// The init function that calls the functions to register a Layer 4.5 customization
// Client will check parameters on first sendmsg
// 0 used as default to skip port or IP checks
// protocol = 256 -> match any layer 4 protocol
// * used as a wildcard for application name to match all [TODO: test]
// @post Layer 4.5 customization registered
int __init sample_client_start(void)
{
    char thread_name[16] = "python3";
    char application_name[16] = "python3";
    int result;


    python_cust = kmalloc(sizeof(struct customization_node), GFP_KERNEL);
    if (python_cust == NULL)
    {
        trace_printk("client kmalloc failed\n");
        return -1;
    }

    // provide pointer for DCA to toggle bypass mode instead of new function
    python_cust->bypass_mode = &bypass;

    // provide pointer for DCA to update priority instead of new function
    python_cust->cust_priority = &priority;


    python_cust->target_flow.protocol = 17; // UDP
                                            // python_cust->protocol = 6; // TCP
                                            // python_cust->protocol = 256; // Any since not valid IP field value
    memcpy(python_cust->target_flow.task_name_pid, thread_name, TASK_NAME_LEN);
    memcpy(python_cust->target_flow.task_name_tgid, application_name, TASK_NAME_LEN);

    // Client: no source IP or port set unless client called bind first
    python_cust->target_flow.dest_port = 65432;
    python_cust->target_flow.source_port = 0;

    // IP is a __be32 value
    python_cust->target_flow.dest_ip = in_aton("127.0.0.1");
    python_cust->target_flow.source_ip = 0;

    // These functions must be defined and will be used to modify the
    // buffer on a send/receive call
    // Function can be set to NULL if not modifying buffer contents
    python_cust->send_function = modify_buffer_send;
    python_cust->recv_function = modify_buffer_recv;
    python_cust->challenge_function = challenge_response;

    // Cust ID set by customization controller, network uniqueness required
    python_cust->cust_id = module_id;

    result = register_customization(python_cust, applyNow);

    if (result != 0)
    {
        trace_printk("L4.5 AELRT: Module failed registration, check debug logs\n");
        return -1;
    }

    trace_printk("L4.5: client module loaded, id=%d\n", python_cust->cust_id);

    hex_to_bytes(hex_key, byte_key, SYMMETRIC_KEY_LENGTH);

    return 0;
}


// Calls the functions to unregister customization node from use on sockets
// @post Layer 4.5 customization node unregistered
void __exit sample_client_end(void)
{
    // NOTE: this is only valid/safe place to call unregister (deadlock scenario)
    int ret = unregister_customization(python_cust);

    if (ret == 0)
    {
        trace_printk("L4.5 AELRT: client module unload error\n");
    }
    else
    {
        trace_printk("L4.5: client module unloaded\n");
    }
    kfree(python_cust);
    return;
}



module_init(sample_client_start);
module_exit(sample_client_end);
MODULE_AUTHOR("Dan Lukaszewski");
MODULE_LICENSE("GPL");
