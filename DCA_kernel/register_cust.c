// @file register_cust.c
// @ brief The functions to register customization modules to be used for socket
// customization at Layer 4.5

#include <linux/list.h>
#include <linux/slab.h> // For allocations
#include <linux/timekeeping.h>

#include "register_cust.h"
#include "customization_socket.h"
#include "util/compare.h" //for comparison functions
#include "util/printing.h"
#include "util/json-maker.h"


// cust list should be short, so linked list should be fast (for now)
// creates linked list head
struct list_head active_customization_list;
static DEFINE_SPINLOCK(active_customization_list_lock);


// this should be a short list that holds cust in transition state
struct list_head deprecated_customization_list;
static DEFINE_SPINLOCK(deprecated_customization_list_lock);


// this should be a short list that is purged often
struct list_head revoked_customization_list;
static DEFINE_SPINLOCK(revoked_customization_list_lock);


int register_customization(struct customization_node *module_cust, bool applyNow);
EXPORT_SYMBOL_GPL(register_customization);

int unregister_customization(struct customization_node *module_cust);
EXPORT_SYMBOL_GPL(unregister_customization);



void init_customization_list(void)
{
  INIT_LIST_HEAD(&active_customization_list);
  INIT_LIST_HEAD(&deprecated_customization_list);
  INIT_LIST_HEAD(&revoked_customization_list);
	return;
}


void free_customization_list(void)
{
	struct customization_node *cust;
	struct customization_node *cust_next;

	spin_lock(&active_customization_list_lock);
	list_for_each_entry_safe(cust, cust_next, &active_customization_list, cust_list_member)
	{
     list_del(&cust->cust_list_member);
  }
	spin_unlock(&active_customization_list_lock);
	return;
}


void free_deprecated_customization_list(void)
{
	struct customization_node *cust;
	struct customization_node *cust_next;

	spin_lock(&deprecated_customization_list_lock);
	list_for_each_entry_safe(cust, cust_next, &deprecated_customization_list, deprecated_cust_list_member)
	{
     list_del(&cust->deprecated_cust_list_member);
  }
	spin_unlock(&deprecated_customization_list_lock);
	return;
}


void free_revoked_customization_list(void)
{
	struct customization_node *cust;
	struct customization_node *cust_next;

	spin_lock(&revoked_customization_list_lock);
	list_for_each_entry_safe(cust, cust_next, &revoked_customization_list, revoked_cust_list_member)
	{
     list_del(&cust->revoked_cust_list_member);
  }
	spin_unlock(&revoked_customization_list_lock);
	return;
}


struct customization_node *get_cust_by_id(u16 cust_id)
{
  struct customization_node *cust_temp = NULL;
	struct customization_node *cust_next = NULL;

	list_for_each_entry_safe(cust_temp, cust_next, &active_customization_list, cust_list_member)
 	{
		if(cust_temp->cust_id == cust_id)
    {
      return cust_temp;
    }
  }
  // cust may be in the deprecated table, but still attached to a relevant socket
  list_for_each_entry_safe(cust_temp, cust_next, &deprecated_customization_list, deprecated_cust_list_member)
 	{
		if(cust_temp->cust_id == cust_id)
    {
      return cust_temp;
    }
  }
  return NULL;
}


struct customization_node *get_customization(struct customization_socket *cust_socket)
{
	struct customization_node *cust_temp = NULL;
	struct customization_node *cust_next = NULL;

	list_for_each_entry_safe(cust_temp, cust_next, &active_customization_list, cust_list_member)
 	{
		if(customization_compare(cust_temp, cust_socket))
		{
      #ifdef DEBUG1
        trace_printk("L4.5: cust socket match to registered module, pid = %d\n", cust_socket->pid);
	    #endif
      return cust_temp;
		}
	}
	return NULL;
}





int register_customization(struct customization_node *module_cust, bool applyNow)
{
  struct customization_node *cust = kmalloc(sizeof(struct customization_node), GFP_KERNEL);
  struct customization_node *test_cust = NULL;

  if(cust == NULL)
	{
		#ifdef DEBUG
			trace_printk("L4.5: kmalloc failed when registering customization node\n");
		#endif
		return -1;
	}

  // invalid customization recieved, at least one function must be valid
  if(module_cust->send_function == NULL && module_cust->recv_function == NULL)
  {
    #ifdef DEBUG
			trace_printk("L4.5 ALERT: NULL registration function check failed\n");
		#endif
    return -1;
  }

  // Verify the ID is unique before allowing registration
  test_cust = get_cust_by_id(module_cust->cust_id);
  if(test_cust != NULL)
  {
    #ifdef DEBUG
      trace_printk("L4.5 ALERT: Duplicate ID check failed\n");
    #endif
    return -1;
  }

  cust->cust_id = module_cust->cust_id;
  cust->sock_count = 0; // init value

  cust->target_flow.protocol = module_cust->target_flow.protocol;
  memcpy(cust->target_flow.task_name_pid, module_cust->target_flow.task_name_pid, TASK_NAME_LEN);
  memcpy(cust->target_flow.task_name_tgid, module_cust->target_flow.task_name_tgid, TASK_NAME_LEN);
  cust->target_flow.dest_port = module_cust->target_flow.dest_port;
  cust->target_flow.source_port = module_cust->target_flow.source_port;
  cust->target_flow.dest_ip = module_cust->target_flow.dest_ip;
  cust->target_flow.source_ip = module_cust->target_flow.source_ip;

  cust->send_function = module_cust->send_function;
  cust->recv_function = module_cust->recv_function;
  cust->challenge_function = module_cust->challenge_function;

  ktime_get_real_ts64(&cust->registration_time_struct);
  cust->deprecated_time_struct.tv_sec = 0;
  cust->deprecated_time_struct.tv_nsec = 0;
	cust->revoked_time_struct.tv_sec = 0;
  cust->revoked_time_struct.tv_nsec = 0;

  cust->send_buffer_size = module_cust->send_buffer_size;
  cust->recv_buffer_size = module_cust->recv_buffer_size;

  #ifdef DEBUG
    trace_printk("L4.5: Registering module\n");
    trace_print_module_params(cust);
  #endif

  #ifdef DEBUG1
    trace_printk("L4.5: Registration time: %llu\n", cust->registration_time_struct.tv_sec);
  #endif

	spin_lock(&active_customization_list_lock);
	list_add(&cust->cust_list_member, &active_customization_list);
	spin_unlock(&active_customization_list_lock);


  if(applyNow)
  {
    // now reset sockets in normal table so they check again
    reset_cust_socket_status();
  }


	return 0;
}

// called by the module to completely remove it from use
int unregister_customization(struct customization_node *module_cust)
{
	int found = 0;
  struct customization_node *matching_cust;
	struct customization_node *cust_next;

  #ifdef DEBUG
    trace_printk("L4.5: Unregistering module\n");
  #endif

  // First: delete from list so no new sockets can claim it
	spin_lock(&active_customization_list_lock);
  list_for_each_entry_safe(matching_cust, cust_next, &active_customization_list, cust_list_member)
 	{
      if(matching_cust->cust_id == module_cust->cust_id)
			{
        #ifdef DEBUG1
          trace_print_module_params(matching_cust);
        #endif
				list_del(&matching_cust->cust_list_member);
				found = 1;
				break;
			}
  }
	spin_unlock(&active_customization_list_lock);


  if(found ==0)
  {
    // cust may have been moved to the deprecated list
    spin_lock(&deprecated_customization_list_lock);
    list_for_each_entry_safe(matching_cust, cust_next, &deprecated_customization_list, deprecated_cust_list_member)
   	{
        if(matching_cust->cust_id == module_cust->cust_id)
  			{
          #ifdef DEBUG1
            trace_print_module_params(matching_cust);
          #endif
  				list_del(&matching_cust->deprecated_cust_list_member);
  				found = 1;
  				break;
  			}
    }
  	spin_unlock(&deprecated_customization_list_lock);
  }

  if(found == 0)
  {
    //if still 0, then we have a problem
    #ifdef DEBUG
      trace_printk("L4.5 ALERT: No customization found with id = %u", module_cust->cust_id);
    #endif
    return found;
  }

  // Second: remove customization from active sockets
	if(found == 1 && matching_cust->sock_count != 0)
	{
		remove_customization_from_each_socket(matching_cust);
	}

  // Last: store customization in revoked list and set revoked time
  ktime_get_real_ts64(&matching_cust->revoked_time_struct);

  #ifdef DEBUG1
    trace_printk("L4.5: Revoked time %llu\n", matching_cust->revoked_time_struct.tv_sec);
  #endif

  spin_lock(&revoked_customization_list_lock);
	list_add(&matching_cust->revoked_cust_list_member, &revoked_customization_list);
	spin_unlock(&revoked_customization_list_lock);

	return found;
}


// remove cust from active list
// called by DCA to stop new sockets from using the registered module
// but DCA only has the module ID, not the struct pointer
int remove_customization_from_active_list(u16 cust_id)
{
	int found = 0;
  struct customization_node *module_cust = NULL;
  struct customization_node *matching_cust;
	struct customization_node *cust_next;

  // First: find cust node matching given cust_id
  module_cust = get_cust_by_id(cust_id);

  if(module_cust == NULL)
  {
    #ifdef DEBUG
      trace_printk("L4.5 ALERT: No module matching id = %u\n", cust_id);
    #endif
    return found;
  }

  found = 1;

  #ifdef DEBUG
    trace_printk("L4.5: Removing module from use by new sockets\n");
  #endif

  // Second: delete from list so no new sockets can claim it
	spin_lock(&active_customization_list_lock);
  list_for_each_entry_safe(matching_cust, cust_next, &active_customization_list, cust_list_member)
 	{
    if(matching_cust->cust_id == module_cust->cust_id)
		{
      #ifdef DEBUG1
        trace_print_module_params(matching_cust);
      #endif
			list_del(&matching_cust->cust_list_member);
			break;
		}
  }
	spin_unlock(&active_customization_list_lock);

  // Last: store customization in deprecated list and set deprecated time
  ktime_get_real_ts64(&matching_cust->deprecated_time_struct);

  #ifdef DEBUG1
    trace_printk("L4.5: Deprecated time %llu\n", matching_cust->deprecated_time_struct.tv_sec);
  #endif

  spin_lock(&deprecated_customization_list_lock);
	list_add(&matching_cust->deprecated_cust_list_member, &deprecated_customization_list);
	spin_unlock(&deprecated_customization_list_lock);

	return found;
}





void netlink_cust_report(char *message, size_t *length)
{
  struct customization_node *cust_temp = NULL;
	struct customization_node *cust_next = NULL;
  size_t rem_length = *length;

  message = json_objOpen(message, NULL, &rem_length);
  message = json_arrOpen(message, "Active", &rem_length);

  // trace_printk("length = %lu, rem_length = %lu\n", *length, rem_length);
  list_for_each_entry_safe(cust_temp, cust_next, &active_customization_list, cust_list_member)
 	{
    message = json_objOpen(message, NULL, &rem_length);
    message = json_uint(message, "ID", cust_temp->cust_id, &rem_length);
    // trace_printk("length = %lu, rem_length = %lu\n", *length, rem_length);
    message = json_uint(message, "Count", cust_temp->sock_count, &rem_length);
    message = json_ulong(message, "ts", cust_temp->registration_time_struct.tv_sec, &rem_length);
    message = json_objClose(message, &rem_length);
  }
  // close Active array
  message = json_arrClose(message, &rem_length);


  message = json_arrOpen(message, "Deprecated", &rem_length);

  // trace_printk("length = %lu, rem_length = %lu\n", *length, rem_length);
  list_for_each_entry_safe(cust_temp, cust_next, &deprecated_customization_list, deprecated_cust_list_member)
 	{
    message = json_objOpen(message, NULL, &rem_length);
    message = json_uint(message, "ID", cust_temp->cust_id, &rem_length);
    // trace_printk("length = %lu, rem_length = %lu\n", *length, rem_length);
    message = json_uint(message, "Count", cust_temp->sock_count, &rem_length);
    message = json_ulong(message, "ts", cust_temp->deprecated_time_struct.tv_sec, &rem_length);
    message = json_objClose(message, &rem_length);
  }
  // close Active array
  message = json_arrClose(message, &rem_length);



  message = json_arrOpen(message, "Revoked", &rem_length);
  list_for_each_entry_safe(cust_temp, cust_next, &revoked_customization_list, revoked_cust_list_member)
 	{
    message = json_objOpen(message, NULL, &rem_length);
    message = json_uint(message, "ID", cust_temp->cust_id, &rem_length);
    // trace_printk("length = %lu, rem_length = %lu\n", *length, rem_length);
    message = json_ulong(message, "ts", cust_temp->revoked_time_struct.tv_sec, &rem_length);
    message = json_objClose(message, &rem_length);
  }
  //close revoked array
  message = json_arrClose(message, &rem_length);
  //close all
  message = json_objClose(message, &rem_length);
  message = json_end_message(message, &rem_length);

  // only report this once for now, but need to find better way to clear list after send
  free_revoked_customization_list();

  #ifdef DEBUG2
    trace_printk("L4.5: rem_length = %lu\n", rem_length);
  #endif

  *length = *length - rem_length;

	return;
}



void netlink_challenge_cust(char *message, size_t *length, char *request)
{
  struct customization_node *cust_node = NULL;
  u16 cust_id = 0;
  char *found = NULL;
  char *msg = NULL;
  char response[HEX_RESPONSE_LENGTH] = "";
  char *iv = NULL;
  int result;
  size_t rem_length = *length;

  // request format: CHALLENGE cust_id iv encrypted_msg END
  // strsep should turn this into:
  // CHALLENGE
  // cust_id
  // iv (as hex)
  // encrypted_msg (as hex)
  // END

  // NOTE: this needs to be cleaned up
  if((found = strsep(&request," ")) != NULL )
  {
    // CHALLENGE
    #ifdef DEBUG2
      trace_printk("Found = %s\n",found);
      trace_printk("Remaining = %s\n", request);
    #endif

    if((found = strsep(&request," ")) != NULL )
    {
      // cust_id
      #ifdef DEBUG2
        trace_printk("Found = %s\n",found);
        trace_printk("Remaining = %s\n", request);
      #endif

      result = kstrtou16(found, 10, &cust_id);
      if (result != 0)
      {
        #ifdef DEBUG
          trace_printk("Error getting cust_id from request\n");
        #endif
      }
      else
      {
        if((iv = strsep(&request," ")) != NULL )
        {
          // iv
          #ifdef DEBUG2
            trace_printk("IV = %s\n",iv);
            trace_printk("Remaining = %s\n", request);
          #endif

          if((msg = strsep(&request," ")) != NULL )
          {
            // encrypted message
            #ifdef DEBUG2
              trace_printk("MSG = %s\n",msg);
              trace_printk("Remaining = %s\n", request);
            #endif
          }
        }
      }
    }
  }

  if (cust_id != 0 && iv != NULL && msg != NULL)
  {
    message = json_objOpen(message, NULL, &rem_length);
    message = json_uint(message, "ID", cust_id, &rem_length);

    cust_node = get_cust_by_id(cust_id);
    cust_node->challenge_function(response, iv, msg);

    message = json_nstr(message, "IV", response, HEX_IV_LENGTH, &rem_length);
    message = json_nstr(message, "Response", response + HEX_IV_LENGTH , HEX_RESPONSE_LENGTH - HEX_IV_LENGTH, &rem_length);

    message = json_objClose(message, &rem_length);
    message = json_end_message(message, &rem_length);

    *length = *length - rem_length;
  }

  return;
}



void netlink_deprecate_cust(char *message, size_t *length, char *request)
{
  u16 cust_id = 0;
  char *found = NULL;
  int result;
  size_t rem_length = *length;

  // request format: DEPRECATE cust_id END
  // strsep should turn this into:
  // DEPRECATE
  // cust_id
  // END

  // NOTE: this needs to be cleaned up
  if((found = strsep(&request," ")) != NULL )
  {
    // DEPRECATE
    #ifdef DEBUG2
      trace_printk("Found = %s\n",found);
      trace_printk("Remaining = %s\n", request);
    #endif

    if((found = strsep(&request," ")) != NULL )
    {
      // cust_id
      #ifdef DEBUG2
        trace_printk("Found = %s\n",found);
        trace_printk("Remaining = %s\n", request);
      #endif

      result = kstrtou16(found, 10, &cust_id);
      if (result != 0)
      {
        #ifdef DEBUG
          trace_printk("Error getting cust_id from request\n");
        #endif
      }
    }
  }

  if (cust_id != 0)
  {
    message = json_objOpen(message, NULL, &rem_length);
    message = json_uint(message, "ID", cust_id, &rem_length);

    result = remove_customization_from_active_list(cust_id);

    message = json_uint(message, "Result", result, &rem_length);

    message = json_objClose(message, &rem_length);
    message = json_end_message(message, &rem_length);

    *length = *length - rem_length;
  }

  return;
}
