// @file test_modules/demo_dns_app_tag.c
// @brief The customization module to remove dns app tag for demo

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

#include "../common_structs.h"

static int __init sample_client_start(void);
static void __exit sample_client_end(void);

extern int register_customization(struct customization_node *cust, u16 applyNow);

extern int unregister_customization(struct customization_node *cust);

extern void trace_print_hex_dump(const char *prefix_str, int prefix_type, int rowsize, int groupsize, const void *buf,
                                 size_t len, bool ascii);

extern void set_module_struct_flags(struct customization_buffer *buf, bool flag_set);

// Kernel module parameters with default values
static char *destination_ip = "10.0.0.40";
module_param(destination_ip, charp, 0600); // root only access to change
MODULE_PARM_DESC(destination_ip, "Dest IP to match");

static char *source_ip = "10.0.0.20";
module_param(source_ip, charp, 0600);
MODULE_PARM_DESC(source_ip, "Dest IP to match");

static unsigned int destination_port = 0;
module_param(destination_port, uint, 0600);
MODULE_PARM_DESC(destination_port, "DPORT to match");

static unsigned int source_port = 53;
module_param(source_port, uint, 0600);
MODULE_PARM_DESC(source_port, "SPORT to match");

static unsigned int protocol = 17; // UDP
module_param(protocol, uint, 0600);
MODULE_PARM_DESC(protocol, "L4 protocol to match");

char cust_tag_test[21] = "XTAGdig";

struct customization_node *dns_cust;

struct skcipher_def
{
    struct scatterlist sg;
    struct crypto_skcipher *tfm;
    struct skcipher_request *req;
    struct crypto_wait wait;
};

// NCO VARIABLES GO HERE
u16 module_id = 1;
char hex_key[HEX_KEY_LENGTH] = "";
u16 activate = 0;
u16 priority = 0;
u16 applyNow = 0;


// END NCO VARIABLES

// established during init function from hex key added during make process
u8 byte_key[SYMMETRIC_KEY_LENGTH] = "";



void modify_buffer_send(struct customization_buffer *send_buf_st, struct customization_flow *socket_flow)
{
    send_buf_st->copy_length = 0;

    set_module_struct_flags(send_buf_st, false);

    // if module hasn't been activated, then don't perform customization
    if (*dns_cust->active_mode == 0)
    {
        send_buf_st->try_next = true;
        return;
    }

    send_buf_st->no_cust = true;
    return;
}



void modify_buffer_recv(struct customization_buffer *recv_buf_st, struct customization_flow *socket_flow)
{
    bool copy_success;
    char tag[5] = "XTAG";
    size_t cust_tag_test_size = (size_t)sizeof(cust_tag_test) - 1; // i.e., 20 bytes

    set_module_struct_flags(recv_buf_st, false);

    // if module hasn't been activated, then don't perform customization
    if (*dns_cust->active_mode == 0)
    {
        recv_buf_st->try_next = true;
        return;
    }


    recv_buf_st->copy_length = 0;

    trace_print_hex_dump("Cust DNS packet recv: ", DUMP_PREFIX_ADDRESS, 16, 1, recv_buf_st->src_iter->iov->iov_base,
                         recv_buf_st->recv_return, true);

    if (strncmp((char *)recv_buf_st->src_iter->iov->iov_base, tag, 4) == 0)
    {
        iov_iter_advance(recv_buf_st->src_iter, cust_tag_test_size);
        copy_success =
            copy_from_iter_full(recv_buf_st->buf, recv_buf_st->recv_return - cust_tag_test_size, recv_buf_st->src_iter);
        if (copy_success == false)
        {
            trace_printk("L4.5 ALERT: Failed to copy DNS to cust buffer\n");
            // Scenario 1: keep cust loaded and allow normal msg to be sent
            recv_buf_st->copy_length = 0;
            return;
        }
        recv_buf_st->copy_length = recv_buf_st->recv_return - cust_tag_test_size;

        // trace_print_hex_dump("DNS packet recv: ", DUMP_PREFIX_ADDRESS, 16, 1, recv_buf_st->buf,
        // recv_buf_st->copy_length, true);
    }

    else
    {
        // something strange came in
        trace_printk("L4.5: DNS packet does not match pattern, size = %lu\n", recv_buf_st->recv_return);
    }

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
// @post Layer 4.5 customization registered
int __init sample_client_start(void)
{
    int result;
    char thread_name[16] = "dnsmasq";
    char application_name[16] = "dnsmasq";

    dns_cust = kmalloc(sizeof(struct customization_node), GFP_KERNEL);
    if (dns_cust == NULL)
    {
        trace_printk("L4.5 ALERT: server dns kmalloc failed\n");
        return -1;
    }

    // provide pointer for DCA to toggle active mode instead of new function
    dns_cust->active_mode = &activate;

    // provide pointer for DCA to update priority instead of new function
    dns_cust->cust_priority = &priority;

    dns_cust->target_flow.protocol = protocol;
    memcpy(dns_cust->target_flow.task_name_pid, thread_name, TASK_NAME_LEN);
    memcpy(dns_cust->target_flow.task_name_tgid, application_name, TASK_NAME_LEN);

    dns_cust->target_flow.dest_port = (u16)destination_port;
    // dnsmasq doesn't bind unless you force it, which I do
    dns_cust->target_flow.dest_ip = in_aton(destination_ip);
    dns_cust->target_flow.source_ip = in_aton(source_ip);
    dns_cust->target_flow.source_port = (u16)source_port;

    dns_cust->send_function = modify_buffer_send;
    dns_cust->recv_function = modify_buffer_recv;
    dns_cust->challenge_function = challenge_response;

    dns_cust->send_buffer_size = 4096;
    dns_cust->recv_buffer_size = 4096;

    // Cust ID set by customization controller, network uniqueness required
    dns_cust->cust_id = module_id;
    dns_cust->registration_time_struct.tv_sec = 0;
    dns_cust->registration_time_struct.tv_nsec = 0;
    dns_cust->revoked_time_struct.tv_sec = 0;
    dns_cust->revoked_time_struct.tv_nsec = 0;

    result = register_customization(dns_cust, applyNow);

    if (result != 0)
    {
        trace_printk("L4.5 ALERT: Module failed registration, check debug logs\n");
        return -1;
    }

    trace_printk("L4.5: server dns module loaded, id=%d\n", dns_cust->cust_id);

    hex_to_bytes(hex_key, byte_key, SYMMETRIC_KEY_LENGTH);

    return 0;
}


// The end function that calls the functions to unregister and stop Layer 4.5
// @post Layer 4.5 customization unregistered
void __exit sample_client_end(void)
{
    int ret = unregister_customization(dns_cust);

    if (ret == 0)
    {
        trace_printk("L4.5 ALERT: server module unload error\n");
    }
    else
    {
        trace_printk("L4.5: server module unloaded\n");
    }
    kfree(dns_cust);
    return;
}


module_init(sample_client_start);
module_exit(sample_client_end);
MODULE_AUTHOR("Dan Lukaszewski");
MODULE_LICENSE("GPL");
