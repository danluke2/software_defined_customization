// @file customization_socket.c
// @brief The functions to establish socket customization at Layer 4.5


#include <linux/hashtable.h>
#include <linux/slab.h>
#include <linux/socket.h>
#include <linux/uio.h> // For iovec structures
#include <net/inet_sock.h>
#include <linux/timekeeping.h> // for timestamps

#include "customization_socket.h"
#include "register_cust.h" //for get_customization
#include "util/printing.h"


unsigned int socket_allocsminusfrees;
#define HASH_TABLE_BITSIZE 10

static DEFINE_HASHTABLE(cust_socket_table, HASH_TABLE_BITSIZE);
static DEFINE_HASHTABLE(normal_socket_table, HASH_TABLE_BITSIZE);


static DEFINE_SPINLOCK(cust_socket_lock);
static DEFINE_SPINLOCK(normal_socket_lock);


// helpers
static void set_send_four_tuple(struct sock *sk, struct customization_socket *cust_socket, struct msghdr *msg);
static void set_recv_four_tuple(struct sock *sk, struct customization_socket *cust_socket, struct msghdr *msg);
static void assign_customization(struct customization_socket *cust_sock, struct customization_node *cust_node);
static void unassign_customization(struct customization_socket *cust_sock);


void init_socket_tables(void)
{
	socket_allocsminusfrees = 0;
	hash_init(cust_socket_table);
	hash_init(normal_socket_table);
	return;
}


struct customization_socket *create_cust_socket(pid_t pid, struct sock *sk, struct msghdr *msg, char *name, int direction)
{
	struct customization_socket *new_cust_socket = NULL;
	struct customization_node *cust_node = NULL;

	new_cust_socket = kmalloc(sizeof(struct customization_socket), GFP_KERNEL);
	if(new_cust_socket == NULL)
	{
		#ifdef DEBUG
			trace_printk("kmalloc failed when creating customization socket\n");
		#endif
		return NULL;
	}
	socket_allocsminusfrees++;

	new_cust_socket->pid = pid; //likely 4 bytes long (depends on system)
	new_cust_socket->sk = sk;
	new_cust_socket->hash_key = (unsigned long)sk;
	new_cust_socket->socket_flow.protocol = sk->sk_protocol;
	memcpy(new_cust_socket->socket_flow.task_name, name, TASK_NAME_LEN);
	// default pointer values to prevent random errors
	new_cust_socket->customization = NULL;
	new_cust_socket->send_buf_st.buf = NULL;
	new_cust_socket->recv_buf_st.buf = NULL;

	// set default time values to indicate no packets seen yet
	new_cust_socket->last_cust_send_time_struct.tv_sec = 0;
	new_cust_socket->last_cust_send_time_struct.tv_nsec = 0;
	new_cust_socket->last_cust_recv_time_struct.tv_sec = 0;
	new_cust_socket->last_cust_recv_time_struct.tv_nsec = 0;

	switch (direction) {
		case SEND:
			set_send_four_tuple(sk, new_cust_socket, msg);
			break;
		case RECV:
			set_recv_four_tuple(sk, new_cust_socket, msg);
			break;
	}
	// all necessary values set to match a customization
	cust_node = get_customization(new_cust_socket);
	if(cust_node == NULL)
	{
		#ifdef DEBUG1
			trace_printk("L4.5: cust request lookup NULL, proto=%u, task=%s\n", sk->sk_protocol, name);
		#endif
		new_cust_socket->customize_send_or_skip = SKIP;
		new_cust_socket->customize_recv_or_skip = SKIP;
	}
	else
	{
		#ifdef DEBUG
			trace_printk("L4.5: Assigning cust to socket, pid %d\n", pid);
		#endif
		assign_customization(new_cust_socket, cust_node);
	}

	if(new_cust_socket->customization == NULL)
	{
		#ifdef DEBUG1
			trace_printk("L4.5: Adding pid %d to normal table\n", pid);
		#endif
		spin_lock(&normal_socket_lock);
		hash_add(normal_socket_table, &new_cust_socket->socket_hash, new_cust_socket->hash_key);
		spin_unlock(&normal_socket_lock);
	}
	else
	{
		#ifdef DEBUG1
			trace_printk("L4.5: Adding pid %d to customization table\n", pid);
		#endif
		spin_lock(&cust_socket_lock);
		hash_add(cust_socket_table, &new_cust_socket->socket_hash, new_cust_socket->hash_key);
		spin_unlock(&cust_socket_lock);
	}
	return new_cust_socket;
}


// Hash for each possible b/c we know hash key and can limit search
struct customization_socket *get_cust_socket(pid_t pid, struct sock *sk)
{
	struct customization_socket *cust_socket = NULL;
	struct customization_socket *cust_socket_iterator;
	unsigned long key = (unsigned long)sk;

	// expedite normal over cust
	spin_lock(&normal_socket_lock);
	hash_for_each_possible(normal_socket_table, cust_socket_iterator, socket_hash, key)
	{
    if (cust_socket_iterator->sk == sk)
		{
			cust_socket = cust_socket_iterator;
			#ifdef DEBUG1
	      trace_printk("L4.5: cust socket found in normal table, current pid %d, cust_pid %d\n", pid, cust_socket_iterator->pid);
	    #endif
      break;
    }
  }
	spin_unlock(&normal_socket_lock);

	if(cust_socket == NULL)
	{
		spin_lock(&cust_socket_lock);
		hash_for_each_possible(cust_socket_table, cust_socket_iterator, socket_hash, key)
		{
	    if (cust_socket_iterator->sk == sk)
			{
	      cust_socket = cust_socket_iterator;
				#ifdef DEBUG1
		      trace_printk("L4.5: cust socket found in customization table, current pid %d, cust_pid %d\n", pid, cust_socket_iterator->pid);
		    #endif
	      break;
	    }
	  }
		spin_unlock(&cust_socket_lock);
	}
	return cust_socket;
}


// Hash for each b/c we need to find matching sockets, so don't know keys
void remove_customization_from_each_socket(struct customization_node *cust)
{
	u32 cust_id = cust->cust_id;
	struct customization_socket *cust_socket_iterator;
	struct customization_node *cust_node;
	int bucket;
	struct hlist_node tmp;
	struct hlist_node* tmpptr = &tmp;

	// Prevent customized socket access while potentially removing cust from it
	spin_lock(&cust_socket_lock);
	hash_for_each_safe(cust_socket_table, bucket, tmpptr, cust_socket_iterator, socket_hash)
	{
		cust_node = cust_socket_iterator->customization;
    if(cust_node != NULL)
		{
			if(cust_node->cust_id == cust_id)
			{
				#ifdef DEBUG1
					trace_printk("L4.5: Found socket mathching cust id=%d\n", cust_id);
				#endif
				spin_lock(&cust_socket_iterator->active_customization_lock);
				unassign_customization(cust_socket_iterator);
				spin_unlock(&cust_socket_iterator->active_customization_lock);

				// Remove socket from customization list
				hash_del(&cust_socket_iterator->socket_hash);

				// Now add to the normal list
				spin_lock(&normal_socket_lock);
				hash_add(normal_socket_table, &cust_socket_iterator->socket_hash, cust_socket_iterator->hash_key);
				spin_unlock(&normal_socket_lock);
			}
    }
  }
	spin_unlock(&cust_socket_lock);
	return;
}


// Only called by Socket close function
int delete_cust_socket(pid_t pid, struct sock *sk)
{
	int found = 0;
	struct customization_socket *cust_socket;
	cust_socket = get_cust_socket(pid,sk);
	if(cust_socket != NULL)
	{
		found = 1;
		if(cust_socket->customization == NULL)
		{
			spin_lock(&normal_socket_lock);
			hash_del(&cust_socket->socket_hash);
			kfree(cust_socket);
			socket_allocsminusfrees--;
			spin_unlock(&normal_socket_lock);
		}
		else
		{
			spin_lock(&cust_socket_lock);
			hash_del(&cust_socket->socket_hash);
			// if socket close called on a customized socket then we allocated
			// buffer space for it
			unassign_customization(cust_socket);
			kfree(cust_socket);
			socket_allocsminusfrees--;
			spin_unlock(&cust_socket_lock);
		}
	}
	return found;
}


// Called when unloading DCA, so no customization modules can be loaded
void delete_all_cust_socket(void)
{
	int bucket;
	struct customization_socket *cust_socket;
	struct hlist_node tmp;
	struct hlist_node* tmpptr = &tmp;


	spin_lock(&normal_socket_lock);
	hash_for_each_safe(normal_socket_table, bucket, tmpptr, cust_socket, socket_hash)
	{
		#ifdef DEBUG
			trace_printk("L4.5 Normal Socket: Deleting things in bucket [%d] with pid value %d and socket value %p\n",
											 bucket, cust_socket->pid, cust_socket->sk);
	  #endif
		hash_del(&cust_socket->socket_hash);
		kfree(cust_socket);
		socket_allocsminusfrees--;
	}
	spin_unlock(&normal_socket_lock);


	// nothing should be in this table, but check anyway to be safe
	spin_lock(&cust_socket_lock);
	hash_for_each_safe(cust_socket_table, bucket, tmpptr, cust_socket, socket_hash)
	{
		#ifdef DEBUG
		trace_printk("L4.5 ALERT Cust Socket: Deleting things in bucket [%d] with pid value %d and socket value %p\n",
											 bucket, cust_socket->pid, cust_socket->sk);
	  #endif
		hash_del(&cust_socket->socket_hash);
		kfree(cust_socket);
		socket_allocsminusfrees--;
	}
	spin_unlock(&cust_socket_lock);
	#ifdef DEBUG
		trace_printk("Socket kallocs minus kfrees: %i\n", socket_allocsminusfrees);
	#endif
	return;
}


// If TCP socket, then msg pointer might be NULL because it does not have a msg yet
static void set_send_four_tuple(struct sock *sk, struct customization_socket *cust_socket, struct msghdr *msg)
{
  struct inet_sock *inet = inet_sk(sk);
	if(msg != NULL)
	{
		DECLARE_SOCKADDR(struct sockaddr_in *, sin, msg->msg_name); // for UDP

		if(sin)
		{
			#ifdef DEBUG1
				trace_printk("L4.5: inside sin for send tuple, pid %d\n", cust_socket->pid);
			#endif
			cust_socket->socket_flow.dest_port = ntohs(sin->sin_port);
	    cust_socket->socket_flow.dest_ip = sin->sin_addr.s_addr;
		}
	}
	else
	{
		cust_socket->socket_flow.dest_port = ntohs(sk->sk_dport);
		cust_socket->socket_flow.dest_ip = sk->sk_daddr;
	}

  cust_socket->socket_flow.source_port = ntohs(inet->inet_sport);
  cust_socket->socket_flow.source_ip = inet->inet_saddr;

  return;
}


// If TCP socket, then msg pointer might be NULL because it does not have a msg yet
static void set_recv_four_tuple(struct sock *sk, struct customization_socket *cust_socket, struct msghdr *msg)
{
  struct inet_sock *inet = inet_sk(sk);
	if(msg != NULL)
	{
		DECLARE_SOCKADDR(struct sockaddr_in *, sin, msg->msg_name); // for UDP

		if(sin)
		{
			#ifdef DEBUG1
				trace_printk("L4.5: inside sin for recv tuple, pid %d\n", cust_socket->pid);
			#endif
			cust_socket->socket_flow.source_port = ntohs(sin->sin_port);
			cust_socket->socket_flow.source_ip = sin->sin_addr.s_addr;
		}
	}
	else
	{
		cust_socket->socket_flow.source_port = ntohs(sk->sk_dport);
		cust_socket->socket_flow.source_ip = sk->sk_daddr;
	}

  cust_socket->socket_flow.dest_port = ntohs(inet->inet_sport);
  cust_socket->socket_flow.dest_ip = inet->inet_saddr;

  return;
}


// Assigns a customization to the socket; allocs buffers and sets params
// @param[X] cust_sock The customization socket to assign cust to
// @param[I] cust_node The customization to apply to the socket
// @post customization buffers are allocated and customization params set
static void assign_customization(struct customization_socket *cust_sock,
													struct customization_node *cust_node)
{
	// condition ? value_if_true : value_if_false
	u32 send_buffer_size = (cust_node->send_buffer_size != 0) ? cust_node->send_buffer_size : SEND_BUF_SIZE;
	u32 recv_buffer_size = (cust_node->recv_buffer_size != 0) ? cust_node->recv_buffer_size : RECV_BUF_SIZE;

	#ifdef DEBUG1
		trace_printk("L4.5: Send buffer size = %u\n", send_buffer_size);
		trace_printk("L4.5: Recv buffer size = %u\n", recv_buffer_size);
	#endif

	cust_sock->customization = NULL; //default value

	if(cust_node->send_function == NULL)
	{
		cust_sock->customize_send_or_skip = SKIP;
		#ifdef DEBUG1
			trace_printk("L4.5: Cust send null, pid=%d, name=%s\n", cust_sock->pid, cust_sock->socket_flow.task_name);
		#endif
	}
	else
	{
		cust_sock->send_buf_st.buf = kmalloc(send_buffer_size,  GFP_KERNEL | __GFP_NOFAIL);
		if(cust_sock->send_buf_st.buf == NULL)
		{
			#ifdef DEBUG
				trace_printk("L4.5 ALERT Send cust buffer malloc fail; proto=%u, task=%s\n", cust_sock->socket_flow.protocol, cust_sock->socket_flow.task_name);
			#endif
			cust_sock->customize_send_or_skip = SKIP;
		}
		else
		{
			#ifdef DEBUG1
				trace_printk("L4.5 Assigned send buffer, pid=%d, name=%s\n", cust_sock->pid, cust_sock->socket_flow.task_name);
			#endif
			socket_allocsminusfrees++;
			cust_sock->send_buf_st.buf_size = send_buffer_size;
			cust_sock->customize_send_or_skip = CUSTOMIZE;
			cust_sock->customization = cust_node;
		}
	}

	if(cust_node->recv_function == NULL)
	{
		cust_sock->customize_recv_or_skip = SKIP;
		#ifdef DEBUG1
			trace_printk("L4.5: Cust recv null, pid=%d, name=%s\n", cust_sock->pid, cust_sock->socket_flow.task_name);
		#endif
	}
	else
	{
		cust_sock->recv_buf_st.buf = kmalloc(recv_buffer_size,  GFP_KERNEL | __GFP_NOFAIL);
		if(cust_sock->recv_buf_st.buf == NULL)
		{
			#ifdef DEBUG
				trace_printk("L4.5: Recv cust buffer malloc fail; proto=%u, task=%s\n", cust_sock->socket_flow.protocol, cust_sock->socket_flow.task_name);
			#endif
			cust_sock->customize_recv_or_skip = SKIP;
		}
		else
		{
			#ifdef DEBUG1
				trace_printk("L4.5: Assigned recv buffer, pid=%d, name=%s\n", cust_sock->pid, cust_sock->socket_flow.task_name);
			#endif
			socket_allocsminusfrees++;
			cust_sock->recv_buf_st.buf_size = recv_buffer_size;
			cust_sock->customize_recv_or_skip = CUSTOMIZE;
			cust_sock->customization = cust_node;
		}
	}

	if(cust_sock->customization != NULL)
	{
		// only need active spinlock if customizing socket
		spin_lock_init(&cust_sock->active_customization_lock);
		// increment count here to keep count accurate/easier to manage
		cust_node->sock_count += 1;
		#ifdef DEBUG2
			trace_printk("L4.5: Socket count = %u\n", cust_node->sock_count);
		#endif
	}
	return;
}


// Removes a customization from the socket; resets values and frees buffers used
// @param[I] cust_sock The customization socket to remove any cust from
static void unassign_customization(struct customization_socket *cust_sock)
{
	struct customization_node *cust_node;

	if(cust_sock->send_buf_st.buf != NULL)
	{
		kfree(cust_sock->send_buf_st.buf);
		cust_sock->send_buf_st.buf = NULL;
		#ifdef DEBUG1
			trace_printk("L4.5: Freed send buff, pid=%d, name=%s\n", cust_sock->pid, cust_sock->socket_flow.task_name);
		#endif
		socket_allocsminusfrees--;
	}

	if(cust_sock->recv_buf_st.buf != NULL)
	{
		kfree(cust_sock->recv_buf_st.buf);
		cust_sock->recv_buf_st.buf = NULL;
		#ifdef DEBUG1
			trace_printk("L4.5: Freed recv buff, pid=%d, name=%s\n", cust_sock->pid, cust_sock->socket_flow.task_name);
		#endif
		socket_allocsminusfrees--;
	}
	// adjust the cust node sock count if necessary
	if(cust_sock->customization != NULL)
	{
		cust_node = cust_sock->customization;
		cust_node->sock_count -= 1;
	}
	#ifdef DEBUG1
		trace_printk("L4.5: Cust removed, pid=%d, name=%s\n", cust_sock->pid, cust_sock->socket_flow.task_name);
	#endif

	cust_sock->customize_send_or_skip = SKIP;
	cust_sock->customize_recv_or_skip = SKIP;
	cust_sock->customization = NULL;

	return;
}


// Debug use only
#ifdef DEBUG
void print_all_cust_socket(void)
{
	int bucket;
	struct customization_socket *cust_socket;
	hash_for_each(cust_socket_table, bucket, cust_socket, socket_hash)
	{
		trace_printk("Bucket [%d] has pid=%d, name=%s\n",
											 bucket, cust_socket->pid, cust_socket->socket_flow.task_name);
	}
	return;
}
#endif
