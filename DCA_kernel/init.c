#ifndef KBUILD_MODNAME
#	define KBUILD_MODNAME KBUILD_STR(layer4_5)
#endif
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pid.h>
#include <linux/sched/signal.h>

// netlink
#include <linux/netlink.h>
#include <net/netlink.h>
#include <net/net_namespace.h>

// layer4.5 local includes
#include "tap.h"
#include "customization_socket.h"
#include "register_cust.h"
#include "util/printing.h"

// Kernel module parameters
static char *layer4_5_path = "/usr/lib/modules/$(uname-r)/layer4_5"; //init var for default value
module_param(layer4_5_path, charp, 0000);  //allow overwriting path
MODULE_PARM_DESC(layer4_5_path, "An absolute path to the Layer 4.5 install location");


//netlink testing
#define NETLINK_TESTFAMILY 25
struct sock *socket;
#define NETLINK_REPORT_SIZE 1024


static void nl_receive_request(struct sk_buff *skb) {
	int result = 0;
  	struct nlmsghdr *nlh = (struct nlmsghdr *) skb->data;
	struct sk_buff *skb_out;
  	pid_t pid = nlh->nlmsg_pid; // pid of the sending process
	char *data = (char*)NLMSG_DATA(nlh);
	char *message = NULL;
	size_t message_size = NETLINK_REPORT_SIZE;
	char failure[32]= "Failed to create cust report";


	message = kmalloc(NETLINK_REPORT_SIZE, GFP_KERNEL);
	if(message == NULL)
	{
		#ifdef DEBUG
			trace_printk("L4.5 ALERT: kmalloc failed when creating netlink report\n");
		#endif
		message = failure;
		message_size = 32;
		return;
	}

	#ifdef DEBUG
		trace_printk("L4.5: NLMSG_DATA = %s\n", data);
	#endif

	if(strncmp(data, "CHALLENGE", 9) == 0)
	{
		// Do challenge query
		netlink_challenge_cust(message, &message_size, data);
	}
	else if(strncmp(data, "DEPRECATE", 9) == 0)
	{
		netlink_deprecate_cust(message, &message_size, data);
	}
	else
	{
		// rewrite message size to number of bytes in message that have data
		netlink_cust_report(message, &message_size);
	}

	skb_out = nlmsg_new(message_size, GFP_KERNEL);
	if (!skb_out) {
		trace_printk("L4.5: Failed to allocate a new skb\n");
		return;
	}

	// trace_print_hex_dump("message: ", DUMP_PREFIX_ADDRESS, 16, 1, message, 32, true);
	nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, message_size, 0);
	NETLINK_CB(skb_out).dst_group = 0;
	memcpy(nlmsg_data(nlh), message, message_size);
	#ifdef DEBUG2
		trace_print_hex_dump("message b4 send: ", DUMP_PREFIX_ADDRESS, 16, 1, nlmsg_data(nlh), 32, true);
	#endif
	result = nlmsg_unicast(socket, skb_out, pid);
	#ifdef DEBUG2
		trace_printk("L4.5: NL result = %d\n", result);
	#endif
}




// Sets up layer 4.5 L4 taps, initializes hash tables for state tracking
// @see protocol_taps/xxx_tap.c for register calls
// @see pcm/customization_socket.c for init_cust_socket_table
// @see pcm/register_cust.c for init_customization_list
// @return an error code
int __init layer4_5_start(void)
{
	struct netlink_kernel_cfg config = {
    	.input = nl_receive_request,
  	};

	// if netlink socket fails, then exit before inserting taps
	socket = netlink_kernel_create(&init_net, NETLINK_TESTFAMILY, &config);
	if (socket == NULL) {
		return -1;
	}

	register_tcp_taps();
	register_udp_taps();
	#ifdef DEBUG
		trace_printk("L4.5: TCP/UDP taps registered\n");
	#endif
	init_socket_tables();
	init_customization_list();

	return 0;
}


// Unregister and stop Layer 4.5, cleans up structures and lists/tables
// @see protocol_taps/xxx_tap.c for unregister calls
// @see pcm/customization_socket.c for delete_all_cust_socket
// @see pcm/register_cust.c for free_customization_list
// @post Layer 4.5 unregistered and stopped
void __exit layer4_5_end(void)
{
	unregister_udp_taps();
	unregister_tcp_taps();
	#ifdef DEBUG
		trace_printk("L4.5: TCP/UDP taps unregistered\n");
	#endif

	delete_all_cust_socket();
	free_customization_list();

	if (socket) {
    	netlink_kernel_release(socket);
  	}
	return;
}



module_init(layer4_5_start);
module_exit(layer4_5_end);
MODULE_AUTHOR("Dan Lukaszewski");
MODULE_LICENSE("GPL");
