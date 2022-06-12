// @file test_modules/overhead_test_app_tag_client.c
// @brief The customization module to remove app tag to test overhead

#include <linux/inet.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uio.h> // For iter structures

// ************** STANDARD PARAMS MUST GO HERE ****************
#include <common_structs.h>
#include <printing.h>
// ************** END STANDARD PARAMS ****************


static int __init sample_client_start(void);
static void __exit sample_client_end(void);


extern int register_customization(struct customization_node *cust);

extern int unregister_customization(struct customization_node *cust);

extern void trace_print_hex_dump(const char *prefix_str, int prefix_type, int rowsize, int groupsize, const void *buf,
                                 size_t len, bool ascii);

// Kernel module parameters with default values
static char *destination_ip = "10.0.0.20";
module_param(destination_ip, charp, 0600); // root only access to change
MODULE_PARM_DESC(destination_ip, "Dest IP to match");

static char *source_ip = "0.0.0.0";
module_param(source_ip, charp, 0600);
MODULE_PARM_DESC(source_ip, "Dest IP to match");

static unsigned int destination_port = 8080;
module_param(destination_port, uint, 0600);
MODULE_PARM_DESC(destination_port, "DPORT to match");

static unsigned int source_port = 0;
module_param(source_port, uint, 0600);
MODULE_PARM_DESC(source_port, "SPORT to match");

static unsigned int protocol = 6; // TCP or UDP
module_param(protocol, uint, 0600);
MODULE_PARM_DESC(protocol, "L4 protocol to match");


size_t extra_bytes_copied_from_last_send = 0;
size_t tag_bytes_removed_last_round = 0;

size_t total_bytes_from_server = 0;
size_t app_bytes_from_server = 0;
size_t total_tags = 0;

size_t BYTE_POSIT = 1000;

char cust_tag_test[33] = "XTAGTAGTAGTAGTAGTAGTAGTAGTAGTAGX";

struct customization_node *client_cust;


static size_t buffered_data = 0;  // amount of data buffered after last recv
static size_t buffered_index = 0; // index where buffered data is
static size_t insert_index = 0;   // where to insert new data

// Helpers
void trace_print_cust_iov_params(struct iov_iter *src_iter)
{
    trace_printk("msg iov len = %lu; offset = %lu\n", src_iter->iov->iov_len, src_iter->iov_offset);
    trace_printk("Total amount of data pointed to by the iovec array (count) = %lu\n", src_iter->count);
    trace_printk("Number of iovec structures (nr_segs) = %lu\n", src_iter->nr_segs);
}


// Function to customize the msg sent from the application to layer 4
void modify_buffer_send(struct customization_buffer *send_buf_st, struct customization_flow *socket_flow)
{
    send_buf_st->copy_length = 0;

    return;
}


// Function to customize the msg recieved from L4 prior to delivery to application
// Put all data without tags into cust buffer, then give app requested length bytes
// we use buf_size to track how many bytes left in cust buffer
// @param[I] recv_buf_st Pointer to the recv buffer structure
// @param[I] socket_flow Pointer to the socket flow parameters
// @pre recv_buf_st->src_iter holds app message destined for application
// @post recv_buf holds customized message for DCA to send to app instead
void modify_buffer_recv(struct customization_buffer *recv_buf_st, struct customization_flow *socket_flow)
{
    size_t cust_tag_test_size = (size_t)sizeof(cust_tag_test) - 1; // i.e., 32 bytes
    size_t temp_buff_index = 0;
    size_t i = 0;
    int remaining_length = recv_buf_st->recv_return;
    int loop_length = 0;
    u32 number_of_tags_removed = 0;
    u32 number_of_partial_tags_removed = 0;

    recv_buf_st->copy_length = 0;
    recv_buf_st->no_cust = false;
    recv_buf_st->skip_cust = false;
    recv_buf_st->buffered_bytes = 0;
    insert_index = 0;


    // if data was buffered in previous recv, then shift it to front of buffer
    if (buffered_data > 0)
    {
        // trace_printk("Adjusting Message buffer\n");
        memmove(recv_buf_st->buf, recv_buf_st->buf + buffered_index, buffered_data);
        insert_index += buffered_data;
        buffered_data = 0;
        buffered_index = 0;
        // trace_print_hex_dump("Buffered Message: ", DUMP_PREFIX_ADDRESS, 16, 1, recv_buf_st->buf, insert_index, true);
    }


    // if recvmsg returned error, then see if we have data, otherwise pass back error
    if (recv_buf_st->recv_return < 0)
    {
        // printk(KERN_ALERT "L4.5: recv ret < 0\n");
        // trace_printk("L4.5: recv ret < 0\n");
        // if we have data buffered, then give to app
        if (insert_index > 0)
        {
            // need to adjust recv_return to match buffered data amount for L4.5 recvmsg
            recv_buf_st->recv_return = insert_index;
            goto buffer_and_send;
        }
        // otherwise, pass error back to the app
        return;
    }

    // NOTE: when buffering allowed, recv_return can be 0
    if (recv_buf_st->recv_return == 0)
    {
        // printk(KERN_ALERT "L4.5: recv ret = 0\n");
        // trace_printk("L4.5: recv ret = 0\n");
        goto buffer_and_send;
    }

    total_bytes_from_server += recv_buf_st->recv_return;
    // printk(KERN_INFO
    //        "L4.5: Total bytes from server to cust mod = %lu, insert index = %lu, recvmsg=%d, req_length=%lu\n",
    //        total_bytes_from_server, insert_index, recv_buf_st->recv_return, recv_buf_st->length);
    // trace_printk("L4.5: Total bytes from server to cust mod = %lu, insert index = %lu, recvmsg=%d, req_length=%lu\n",
    //              total_bytes_from_server, insert_index, recv_buf_st->recv_return, recv_buf_st->length);


    if (recv_buf_st->recv_return + insert_index >= recv_buf_st->buf_size)
    {
        // not likely anymore, but possible until I put in a better fix
        trace_printk("L4.5 ALERT: putting too many bytes into buffer\n");
        recv_buf_st->skip_cust = true;
        recv_buf_st->recv_return = 0;
        return;
    }


    // trace_printk("L4.5 Start Params: recvmsg=%d, tag_bytes=%lu, extra_bytes=%lu, total_bytes=%lu, length=%lu, "
    //              "insert_index=%lu\n",
    //              recv_buf_st->recv_return, tag_bytes_removed_last_round, extra_bytes_copied_from_last_send,
    //              total_bytes_from_server, recv_buf_st->length, insert_index);

    // we might have more tag bytes to strip before reaching for loop
    if (tag_bytes_removed_last_round > 0)
    {
        if (remaining_length >= (cust_tag_test_size - tag_bytes_removed_last_round))
        {
            // advance the temp buffer index
            temp_buff_index += cust_tag_test_size - tag_bytes_removed_last_round;

            remaining_length -= (cust_tag_test_size - tag_bytes_removed_last_round);
            number_of_partial_tags_removed += 1;
            total_tags += 1;
            tag_bytes_removed_last_round = 0;
        }
        else
        {
            trace_printk("L4.5 INFO: Hit edge case, just tag bytes left in packet\n");
            goto buffer_and_send;
        }
    }

    // we might have copied some bytes from last round but we did not reach the tag portion yet
    else if (extra_bytes_copied_from_last_send > 0)
    {
        // trace_printk("L4.5: extra = %lu, recv_buf_st->recv_return = %lu\n", extra_bytes_copied_from_last_send,
        //  recv_buf_st->recv_return);
        // we had bytes counting towards BYTE_POSIT from last send call

        // are there enough bytes to reach the tag position?
        if (BYTE_POSIT - extra_bytes_copied_from_last_send > recv_buf_st->recv_return)
        {
            // trace_printk("L4.5: not enough bytes to copy, bytes = %d\n", recv_buf_st->recv_return);
            // copy entire buffer because still under BYTE_POSIT value
            memcpy(recv_buf_st->buf + insert_index, recv_buf_st->temp_buf, recv_buf_st->recv_return);
            insert_index += recv_buf_st->recv_return;

            // trace_printk("L4.5: Copied %d bytes to cust buffer\n", recv_buf_st->recv_return);
            extra_bytes_copied_from_last_send += recv_buf_st->recv_return;

            goto buffer_and_send;
        }
        else
        {
            // trace_printk("L4.5: enough bytes to copy, bytes = %lu\n", BYTE_POSIT -
            // extra_bytes_copied_from_last_send); first copy remaining bytes to reach BYTE_POSIT
            memcpy(recv_buf_st->buf + insert_index, recv_buf_st->temp_buf,
                   BYTE_POSIT - extra_bytes_copied_from_last_send);
            insert_index += BYTE_POSIT - extra_bytes_copied_from_last_send;

            // trace_printk("Copied %lu bytes to cust buffer\n", BYTE_POSIT - extra_bytes_copied_from_last_send);
            temp_buff_index += BYTE_POSIT - extra_bytes_copied_from_last_send;
            remaining_length -= BYTE_POSIT - extra_bytes_copied_from_last_send;
            extra_bytes_copied_from_last_send = 0;

            // now check if enough tag bytes to remove
            if (remaining_length >= cust_tag_test_size)
            {
                // trace_print_hex_dump("Tag Removal Extra: ", DUMP_PREFIX_ADDRESS, 16, 1,
                // recv_buf_st->src_iter->iov->iov_base + recv_buf_st->src_iter->iov_offset, cust_tag_test_size,
                // true);
                temp_buff_index += cust_tag_test_size;
                remaining_length -= cust_tag_test_size;
                number_of_tags_removed += 1;
                total_tags += 1;
            }
            else
            {
                // track how many tag bytes we found for next round
                tag_bytes_removed_last_round = remaining_length;
                number_of_partial_tags_removed += 1;
                goto buffer_and_send;
            }
        }
    }

    loop_length = remaining_length;
    // trace_printk("L4.5: loop length = %d\n", loop_length);
    // at this point we have 0 bytes inserted toward BYTE_POSIT tag positiion
    for (i = 0; i + BYTE_POSIT + cust_tag_test_size <= loop_length; i += BYTE_POSIT + cust_tag_test_size)
    {
        memcpy(recv_buf_st->buf + insert_index, recv_buf_st->temp_buf + temp_buff_index, BYTE_POSIT);
        insert_index += BYTE_POSIT;

        remaining_length -= BYTE_POSIT;
        remaining_length -= cust_tag_test_size;
        number_of_tags_removed += 1;
        total_tags += 1;
        temp_buff_index += BYTE_POSIT + cust_tag_test_size;
    }


    if (remaining_length > 0)
    {
        // trace_printk("L4.5: remaining length = %d\n", remaining_length);
        // if remaining length > BYTE_POSIT but less than BYTE_POSIT + cust_tag_test_size, then have a partial tag to
        // deal with
        if (remaining_length > BYTE_POSIT)
        {
            memcpy(recv_buf_st->buf + insert_index, recv_buf_st->temp_buf + temp_buff_index, BYTE_POSIT);
            insert_index += BYTE_POSIT;

            tag_bytes_removed_last_round = remaining_length - BYTE_POSIT;
            extra_bytes_copied_from_last_send = 0;
            goto buffer_and_send;
        }
        else
        {
            memcpy(recv_buf_st->buf + insert_index, recv_buf_st->temp_buf + temp_buff_index, remaining_length);
            insert_index += remaining_length;

            extra_bytes_copied_from_last_send += remaining_length;
            tag_bytes_removed_last_round = 0;
            goto buffer_and_send;
        }
    }

    // trace_printk("L4.5: Number of tags removed = %u\n", number_of_tags_removed);
    // trace_printk("L4.5: Number of partial tags removed = %u\n", number_of_partial_tags_removed);
    // trace_printk("L4.5: Total tags fully removed = %lu\n", total_tags);

    // app_bytes_from_server += insert_index;
    // trace_printk("L4.5: Total app bytes from server to cust mod = %lu\n", app_bytes_from_server);
    // trace_printk("L4.5 End Params: tag_bytes=%lu, extra_bytes=%lu\n", tag_bytes_removed_last_round,
    // extra_bytes_copied_from_last_send);

    // now we have all app data uncustomized in the cust buffer, so give app what it wants from the buffer

buffer_and_send:
    // printk(KERN_INFO
    //        "L4.5: Total bytes from server to cust mod = %lu, insert index = %lu, recvmsg=%d, req_length=%lu\n",
    //        total_bytes_from_server, insert_index, recv_buf_st->recv_return, recv_buf_st->length);
    // figure out what app can handle now that we put data into the cust buffer
    if (insert_index > recv_buf_st->length)
    {
        // printk(KERN_INFO "L4.5: insert index larger, index = %lu\n", insert_index);
        // trace_printk("L4.5: insert index larger, index = %lu, req_length=%lu\n", insert_index, recv_buf_st->length);
        // give what app wants and buffer the rest
        recv_buf_st->copy_length = recv_buf_st->length;
        buffered_data = insert_index - recv_buf_st->length;
        buffered_index = recv_buf_st->copy_length;
        recv_buf_st->buffered_bytes = buffered_data;
        // printk(KERN_INFO "L4.5: return params, copy_length=%lu, buffered_data=%lu, recvmsg=%d\n",
        //        recv_buf_st->copy_length, buffered_data, recv_buf_st->recv_return);
        // trace_printk("L4.5: return params, copy_length=%lu, buffered_data=%lu, recvmsg=%d\n",
        // recv_buf_st->copy_length,
        //              buffered_data, recv_buf_st->recv_return);
    }
    else
    {
        // printk(KERN_INFO "L4.5: insert index smaller, index = %lu\n", insert_index);
        // trace_printk("L4.5: insert index smaller, index = %lu, req_length=%lu\n", insert_index, recv_buf_st->length);
        // app can take it all
        recv_buf_st->copy_length = insert_index;
    }

    return;
}



// The init function that calls the functions to register a Layer 4.5 customization
// @post Layer 4.5 customization registered
int __init sample_client_start(void)
{
    char thread_name[16] = "curl";
    char application_name[16] = "curl";
    int result;

    client_cust = kmalloc(sizeof(struct customization_node), GFP_KERNEL);
    if (client_cust == NULL)
    {
        trace_printk("L4.5 ALERT: client kmalloc failed\n");
        return -1;
    }

    client_cust->target_flow.protocol = (u16)protocol; // TCP
    memcpy(client_cust->target_flow.task_name_pid, thread_name, TASK_NAME_LEN);
    memcpy(client_cust->target_flow.task_name_tgid, application_name, TASK_NAME_LEN);

    client_cust->target_flow.dest_port = (u16)destination_port;
    client_cust->target_flow.source_port = (u16)source_port;

    client_cust->target_flow.dest_ip = in_aton(destination_ip);
    client_cust->target_flow.source_ip = in_aton(source_ip);

    client_cust->send_function = NULL;
    client_cust->recv_function = modify_buffer_recv;

    client_cust->cust_id = 78;
    client_cust->registration_time_struct.tv_sec = 0;
    client_cust->registration_time_struct.tv_nsec = 0;
    client_cust->retired_time_struct.tv_sec = 0;
    client_cust->retired_time_struct.tv_nsec = 0;

    client_cust->send_buffer_size = 0; //  normal buffer size
    client_cust->recv_buffer_size = 65536 * 2;
    // temp_buffer_size <= recv_buffer_size
    client_cust->temp_buffer_size = 102400 + 3200;
    // curl buffer length from trace messages = 102400
    // there will be at least 3200 tag bytes so we can put more in temp that will just be removed

    result = register_customization(client_cust);

    if (result != 0)
    {
        trace_printk("L4.5 ALERT: Module failed registration, check debug logs\n");
        return -1;
    }

    trace_printk("L4.5: client module loaded, id=%d\n", client_cust->cust_id);

    return 0;
}


// Calls the functions to unregister customization node from use on sockets
// @post Layer 4.5 customization node unregistered
void __exit sample_client_end(void)
{
    // NOTE: this is only valid/safe place to call unregister (deadlock scenario)
    int ret = unregister_customization(client_cust);

    if (ret == 0)
    {
        trace_printk("L4.5 ALERT: client module unload error\n");
    }
    else
    {
        trace_printk("L4.5: client module unloaded\n");
    }
    kfree(client_cust);
    return;
}


module_init(sample_client_start);
module_exit(sample_client_end);
MODULE_AUTHOR("Dan Lukaszewski");
MODULE_LICENSE("GPL");
