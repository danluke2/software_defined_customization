// @file register_cust.c
// @ brief The functions to register customization modules to be used for socket
// customization at Layer 4.5

#include <linux/list.h>
#include <linux/slab.h> // For allocations
#include <linux/sort.h>
#include <linux/timekeeping.h>

#include "customization_socket.h"
#include "register_cust.h"
#include "util/compare.h" //for comparison functions
#include "util/helpers.h"
#include "util/json-maker.h"

// cust list should be short, so linked list should be fast (for now)
// TODO: Is there a better data structure to use?

// creates linked list head
struct list_head installed_customization_list;
static DEFINE_SPINLOCK(installed_customization_list_lock);

// this should be a short list that holds cust in transition state
struct list_head deprecated_customization_list;
static DEFINE_SPINLOCK(deprecated_customization_list_lock);

// this should be a short list that is purged often
struct list_head revoked_customization_list;
static DEFINE_SPINLOCK(revoked_customization_list_lock);

int
register_customization(struct customization_node* module_cust, u16 applyNow);
EXPORT_SYMBOL_GPL(register_customization);

int
unregister_customization(struct customization_node* module_cust);
EXPORT_SYMBOL_GPL(unregister_customization);

void
init_customization_list(void)
{
  INIT_LIST_HEAD(&installed_customization_list);
  INIT_LIST_HEAD(&deprecated_customization_list);
  INIT_LIST_HEAD(&revoked_customization_list);
  return;
}

void
free_customization_list(void)
{
  struct customization_node* cust;
  struct customization_node* cust_next;

  spin_lock(&installed_customization_list_lock);
  list_for_each_entry_safe(
    cust, cust_next, &installed_customization_list, cust_list_member)
  {
    list_del(&cust->cust_list_member);
  }
  spin_unlock(&installed_customization_list_lock);
  return;
}

void
free_deprecated_customization_list(void)
{
  struct customization_node* cust;
  struct customization_node* cust_next;

  spin_lock(&deprecated_customization_list_lock);
  list_for_each_entry_safe(cust,
                           cust_next,
                           &deprecated_customization_list,
                           deprecated_cust_list_member)
  {
    list_del(&cust->deprecated_cust_list_member);
  }
  spin_unlock(&deprecated_customization_list_lock);
  return;
}

void
free_revoked_customization_list(void)
{
  struct customization_node* cust;
  struct customization_node* cust_next;

  spin_lock(&revoked_customization_list_lock);
  list_for_each_entry_safe(
    cust, cust_next, &revoked_customization_list, revoked_cust_list_member)
  {
    list_del(&cust->revoked_cust_list_member);
  }
  spin_unlock(&revoked_customization_list_lock);
  return;
}

struct customization_node*
get_cust_by_id(u16 cust_id)
{
  struct customization_node* cust_temp = NULL;
  struct customization_node* cust_next = NULL;

  list_for_each_entry_safe(
    cust_temp, cust_next, &installed_customization_list, cust_list_member)
  {
    if (cust_temp->cust_id == cust_id) {
      return cust_temp;
    }
  }
  // if we reach this point, then we haven't found the cust module
  // cust may be in the deprecated table, but still attached to a relevant
  // socket
  list_for_each_entry_safe(cust_temp,
                           cust_next,
                           &deprecated_customization_list,
                           deprecated_cust_list_member)
  {
    if (cust_temp->cust_id == cust_id) {
      return cust_temp;
    }
  }
  return NULL;
}

size_t
get_customizations(struct customization_socket* cust_socket,
                   struct customization_node* nodes[MAX_CUST_ATTACH])
{
  struct customization_node* cust_temp = NULL;
  struct customization_node* cust_next = NULL;
  size_t counter = 0;
  u16 priority_threshold = 65535; // max u16 value; TODO: This should be a
                                  // variable instead of hardcoded

  list_for_each_entry_safe(
    cust_temp, cust_next, &installed_customization_list, cust_list_member)
  {
    if (customization_compare(cust_temp, cust_socket)) {
#ifdef DEBUG1
      trace_printk("L4.5: cust socket match to registered module, pid = %d\n",
                   cust_socket->pid);
#endif
      // TODO: It seems like priority_threshold is not used since it is set to
      // max value
      //  if cust priority lower than current threshold, then add to the array
      if (*cust_temp->cust_priority <= priority_threshold) {
        nodes[counter] = cust_temp;
        counter += 1;
        if (counter == MAX_CUST_ATTACH) {
          break;
        }
      }
    }
  }
#ifdef DEBUG1
  trace_printk("L4.5: found %lu nodes for socket\n", counter);
#endif
  // now sort array by priority values before returning
  // counter holds number of modules in nodes array, so we won't reach NULL
  // pointer
  if (counter > 1) {
    sort(nodes, counter, sizeof(void*), &priority_compare, NULL);
  }

  return counter;
}

int
register_customization(struct customization_node* module_cust, u16 applyNow)
{
  struct customization_node* cust =
    kmalloc(sizeof(struct customization_node), GFP_KERNEL);
  struct customization_node* test_cust = NULL;

  if (cust == NULL) {
#ifdef DEBUG
    trace_printk("L4.5: kmalloc failed when registering customization node\n");
#endif
    return -1;
  }

  // invalid customization recieved, both function must be valid (design choice)
  if (module_cust->send_function == NULL ||
      module_cust->recv_function == NULL) {
#ifdef DEBUG
    trace_printk("L4.5 ALERT: NULL registration function check failed\n");
#endif
    return -1;
  }

  // TODO: should DCA set the module ID instead of having module supply it?
  // if supplied, can't rely on NCO to deconflict ID's so must check uniqueness

  // Verify the ID is unique before allowing registration
  test_cust = get_cust_by_id(module_cust->cust_id);
  if (test_cust != NULL) {
#ifdef DEBUG
    trace_printk("L4.5 ALERT: Duplicate ID check failed\n");
#endif
    return -1;
  }

  // TODO: Is there a better way to copy over values?
  //  Should this be a seperate function?
  //  Need to also check memcpy succeeded instead of trusting it did
  cust->cust_id = module_cust->cust_id;
  cust->active_mode = module_cust->active_mode;
  cust->sock_count = 0; // init value
  cust->cust_priority = module_cust->cust_priority;

  cust->target_flow.protocol = module_cust->target_flow.protocol;
  memcpy(cust->target_flow.task_name_pid,
         module_cust->target_flow.task_name_pid,
         TASK_NAME_LEN);
  memcpy(cust->target_flow.task_name_tgid,
         module_cust->target_flow.task_name_tgid,
         TASK_NAME_LEN);
  cust->target_flow.dest_port = module_cust->target_flow.dest_port;
  cust->target_flow.source_port = module_cust->target_flow.source_port;
  cust->target_flow.dest_ip = module_cust->target_flow.dest_ip;
  cust->target_flow.source_ip = module_cust->target_flow.source_ip;

  cust->send_function = module_cust->send_function;
  cust->recv_function = module_cust->recv_function;
  cust->challenge_function = module_cust->challenge_function;

  // TODO: do I need to store these times or just report back to NCO when they
  // happen?
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
  trace_printk("L4.5: Registration time: %llu\n",
               cust->registration_time_struct.tv_sec);
#endif

  spin_lock(&installed_customization_list_lock);
  list_add(&cust->cust_list_member, &installed_customization_list);
  spin_unlock(&installed_customization_list_lock);

  if (applyNow) {
    // now set update check on sockets so they check again
    set_update_cust_check();
  }

  return 0;
}

// called by the module to completely remove it from use
int
unregister_customization(struct customization_node* module_cust)
{
  int found = 0;
  struct customization_node* matching_cust;
  struct customization_node* cust_next;

#ifdef DEBUG
  trace_printk("L4.5: Unregistering module\n");
#endif

  // First: delete from list so no new sockets can claim it
  spin_lock(&installed_customization_list_lock);
  list_for_each_entry_safe(
    matching_cust, cust_next, &installed_customization_list, cust_list_member)
  {
    if (matching_cust->cust_id == module_cust->cust_id) {
#ifdef DEBUG1
      trace_print_module_params(matching_cust);
#endif
      list_del(&matching_cust->cust_list_member);
      found = 1;
      break;
    }
  }
  spin_unlock(&installed_customization_list_lock);

  if (found == 0) {
    // cust may have been moved to the deprecated list
    spin_lock(&deprecated_customization_list_lock);
    list_for_each_entry_safe(matching_cust,
                             cust_next,
                             &deprecated_customization_list,
                             deprecated_cust_list_member)
    {
      if (matching_cust->cust_id == module_cust->cust_id) {
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

  if (found == 0) {
    // if still 0, then we have a problem
#ifdef DEBUG
    trace_printk("L4.5 ALERT: No customization found with id = %u",
                 module_cust->cust_id);
#endif
    return found;
  }

  // TODO: Should I check found==1 since it can't be anything else and reach
  // here

  // Second: remove customization from active sockets
  if (found == 1 && matching_cust->sock_count != 0) {
    remove_customization_from_each_socket(matching_cust);
  }

  // Last: store customization in revoked list and set revoked time
  ktime_get_real_ts64(&matching_cust->revoked_time_struct);

#ifdef DEBUG1
  trace_printk("L4.5: Revoked time %llu\n",
               matching_cust->revoked_time_struct.tv_sec);
#endif

  spin_lock(&revoked_customization_list_lock);
  list_add(&matching_cust->revoked_cust_list_member,
           &revoked_customization_list);
  spin_unlock(&revoked_customization_list_lock);

  return found;
}

// Deprecate will remove cust from active list
// called by DCA to stop new sockets from using the registered module
// but DCA only has the module ID, not the struct pointer
int
deprecate_customization(u16 cust_id)
{
  int found = 0;
  struct customization_node* module_cust = NULL;
  struct customization_node* matching_cust;
  struct customization_node* cust_next;

  // First: find cust node matching given cust_id
  module_cust = get_cust_by_id(cust_id);

  if (module_cust == NULL) {
#ifdef DEBUG
    trace_printk("L4.5 ALERT: No module matching id = %u\n", cust_id);
#endif
    return found;
  }

#ifdef DEBUG
  trace_printk("L4.5: Removing module from use by new sockets\n");
#endif

  // Second: delete from list so no new sockets can claim it
  spin_lock(&installed_customization_list_lock);
  list_for_each_entry_safe(
    matching_cust, cust_next, &installed_customization_list, cust_list_member)
  {
    if (matching_cust->cust_id == module_cust->cust_id) {
#ifdef DEBUG1
      trace_print_module_params(matching_cust);
#endif
      list_del(&matching_cust->cust_list_member);
      found = 1;
      break;
    }
  }
  spin_unlock(&installed_customization_list_lock);

  if (found == 1) {
    // Last: store customization in deprecated list and set deprecated time
    ktime_get_real_ts64(&matching_cust->deprecated_time_struct);

#ifdef DEBUG1
    trace_printk("L4.5: Deprecated time %llu\n",
                 matching_cust->deprecated_time_struct.tv_sec);
#endif

    spin_lock(&deprecated_customization_list_lock);
    list_add(&matching_cust->deprecated_cust_list_member,
             &deprecated_customization_list);
    spin_unlock(&deprecated_customization_list_lock);
  }

  else {
#ifdef DEBUG
    trace_printk("L4.5: Did not successfully deprecate module\n");
#endif
  }

  return found;
}

// set customization active mode
int
toggle_customization_active_mode(u16 cust_id, u16 mode)
{
  int found = 0;
  struct customization_node* module_cust = NULL;

  // First: find cust node matching given cust_id
  module_cust = get_cust_by_id(cust_id);

  if (module_cust != NULL) {
    found = 1;
    *module_cust->active_mode = mode;
  }

  else {
#ifdef DEBUG
    trace_printk("L4.5 ALERT: No module matching id = %u\n", cust_id);
#endif
  }

  return found;
}

// set customization priority
int
set_customization_priority(u16 cust_id, u16 priority)
{
  int found = 0;
  struct customization_node* module_cust = NULL;

  // First: find cust node matching given cust_id
  module_cust = get_cust_by_id(cust_id);

  if (module_cust != NULL) {
    found = 1;
    *module_cust->cust_priority = priority;

    // now we need to sort sockets with module attached again since order may
    // change
    if (module_cust->sock_count > 0) {
      // mark socket for sorting when next used
      sort_mark_each_socket_with_matching_cust(module_cust);
    }
  }

  else {
#ifdef DEBUG
    trace_printk("L4.5 ALERT: No module matching id = %u\n", cust_id);
#endif
  }

  return found;
}

// Split request string into word array of char pointers
// number_of_words >= 1
int
split_message(char* request, char* words[], u16 number_of_words)
{
  int word_index = 0;
  char* aPtr;

  do {
    aPtr = strsep(&request, " ");
    if (aPtr != NULL) {
#ifdef DEBUG2
      trace_printk("Found = %s\n", aPtr);
#endif
      words[word_index] = aPtr;
      word_index += 1;
    }
  } while (aPtr != NULL && word_index < number_of_words);

  return word_index;
}

void
netlink_cust_report(char* message, size_t* length)
{
  struct customization_node* cust_temp = NULL;
  struct customization_node* cust_next = NULL;
  size_t rem_length = *length;

  message = json_objOpen(message, NULL, &rem_length);
  message = json_arrOpen(message, "Installed", &rem_length);

  // trace_printk("length = %lu, rem_length = %lu\n", *length, rem_length);
  list_for_each_entry_safe(
    cust_temp, cust_next, &installed_customization_list, cust_list_member)
  {
    message = json_objOpen(message, NULL, &rem_length);
    message = json_uint(message, "ID", cust_temp->cust_id, &rem_length);
    message =
      json_uint(message, "Activated", *cust_temp->active_mode, &rem_length);
    message = json_uint(message, "Count", cust_temp->sock_count, &rem_length);
    message = json_ulong(
      message, "ts", cust_temp->registration_time_struct.tv_sec, &rem_length);
    message = json_objClose(message, &rem_length);
  }
  // close Active array
  message = json_arrClose(message, &rem_length);

  message = json_arrOpen(message, "Deprecated", &rem_length);

  // trace_printk("length = %lu, rem_length = %lu\n", *length, rem_length);
  list_for_each_entry_safe(cust_temp,
                           cust_next,
                           &deprecated_customization_list,
                           deprecated_cust_list_member)
  {
    message = json_objOpen(message, NULL, &rem_length);
    message = json_uint(message, "ID", cust_temp->cust_id, &rem_length);
    message =
      json_uint(message, "Activated", *cust_temp->active_mode, &rem_length);
    message = json_uint(message, "Count", cust_temp->sock_count, &rem_length);
    message = json_ulong(
      message, "ts", cust_temp->deprecated_time_struct.tv_sec, &rem_length);
    message = json_objClose(message, &rem_length);
  }
  // close Active array
  message = json_arrClose(message, &rem_length);

  message = json_arrOpen(message, "Revoked", &rem_length);
  list_for_each_entry_safe(
    cust_temp, cust_next, &revoked_customization_list, revoked_cust_list_member)
  {
    message = json_objOpen(message, NULL, &rem_length);
    message = json_uint(message, "ID", cust_temp->cust_id, &rem_length);
    // trace_printk("length = %lu, rem_length = %lu\n", *length, rem_length);
    message = json_ulong(
      message, "ts", cust_temp->revoked_time_struct.tv_sec, &rem_length);
    message = json_objClose(message, &rem_length);
  }
  // close revoked array
  message = json_arrClose(message, &rem_length);
  // close all
  message = json_objClose(message, &rem_length);
  message = json_end_message(message, &rem_length);

  // TODO: only report this once for now, but need to find better way to clear
  // list after send
  free_revoked_customization_list();

  *length = *length - rem_length;

  return;
}

// TODO: All these netlink messages follow similar processing.  Can they be
// better combined?

void
netlink_challenge_cust(char* message, size_t* length, char* request)
{
  struct customization_node* cust_node = NULL;
  u16 cust_id = 0;
  int word_count = 0;
  int expected_words = 5; // TODO: this shouldn't be a magic number
  char* words[5];         // hold expected_words pointers to char pointers
  char* msg = NULL;
  char response[HEX_RESPONSE_LENGTH] = "";
  char* iv = NULL;
  int result;
  size_t rem_length = *length;

  // request format: CHALLENGE cust_id iv encrypted_msg END
  // split_message should turn this into:
  // CHALLENGE
  // cust_id
  // iv (as hex)
  // encrypted_msg (as hex)
  // END

  word_count = split_message(request, words, expected_words);

  if (word_count != expected_words) {
    trace_printk("L4.5 ALERT: challenge word count error = %d\n", word_count);
    // we did not get a valid message, so return error message
    goto parsing_error_msg;
  }

  result = kstrtou16((words[1]), 10, &cust_id);
  if (result != 0) {
    trace_printk(
      "L4.5 ALERT: challenge cust_id %s, error = %d\n", words[1], result);
    // cust id string to u16 failed, so return error message
    goto parsing_error_msg;
  }

  // TODO: these shouldn't be magic numbers
  iv = words[2];
  msg = words[3];

  message = json_objOpen(message, NULL, &rem_length);
  message = json_uint(message, "ID", cust_id, &rem_length);

  cust_node = get_cust_by_id(cust_id);
  // modules don't require a challenge function at registration time (design
  // choice)
  if (cust_node->challenge_function != NULL) {
    cust_node->challenge_function(response, iv, msg);

    message = json_nstr(message, "IV", response, HEX_IV_LENGTH, &rem_length);
    message = json_nstr(message,
                        "Response",
                        response + HEX_IV_LENGTH,
                        HEX_RESPONSE_LENGTH - HEX_IV_LENGTH,
                        &rem_length);

    message = json_objClose(message, &rem_length);
    message = json_end_message(message, &rem_length);

    *length = *length - rem_length;
  } else {
    goto function_error_msg;
  }

  return;

// TODO: Messages and lengths should be variables
parsing_error_msg:
  message = json_objOpen(message, NULL, &rem_length);
  message = json_nstr(message, "ERROR", "parsing", 7, &rem_length);
  message = json_objClose(message, &rem_length);
  message = json_end_message(message, &rem_length);
  *length = *length - rem_length;
  return;

function_error_msg:
  message = json_objOpen(message, NULL, &rem_length);
  message =
    json_nstr(message, "ERROR", "NULL challenge function", 23, &rem_length);
  message = json_objClose(message, &rem_length);
  message = json_end_message(message, &rem_length);
  *length = *length - rem_length;
  return;
}

void
netlink_deprecate_cust(char* message, size_t* length, char* request)
{
  u16 cust_id = 0;
  int word_count = 0;
  int expected_words = 3;
  char* words[3]; // hold expected_words pointers to char pointers
  int result;
  size_t rem_length = *length;

  // request format: DEPRECATE cust_id END
  // split_message should turn this into:
  // DEPRECATE
  // cust_id
  // END

  word_count = split_message(request, words, expected_words);

  if (word_count != expected_words) {
    trace_printk("L4.5 ALERT: challenge word count error = %d\n", word_count);
    // we did not get a valid message, so return error message
    goto error_msg;
  }

  result = kstrtou16((words[1]), 10, &cust_id);
  if (result != 0) {
    trace_printk(
      "L4.5 ALERT: challenge cust_id %s, error = %d\n", words[1], result);
    // cust id string to u16 failed, so return error message
    goto error_msg;
  }

  message = json_objOpen(message, NULL, &rem_length);
  message = json_uint(message, "ID", cust_id, &rem_length);

  result = deprecate_customization(cust_id);

  message = json_uint(message, "Result", result, &rem_length);

  message = json_objClose(message, &rem_length);
  message = json_end_message(message, &rem_length);

  *length = *length - rem_length;

  return;

// TODO: Messages and lengths should be variables
error_msg:
  message = json_objOpen(message, NULL, &rem_length);
  message = json_nstr(message, "ERROR", "parsing", 7, &rem_length);
  message = json_objClose(message, &rem_length);
  message = json_end_message(message, &rem_length);
  *length = *length - rem_length;
  return;
}

void
netlink_toggle_cust(char* message, size_t* length, char* request)
{
  u16 cust_id = 0;
  int word_count = 0;
  int expected_words = 4;
  char* words[4]; // hold expected_words pointers to char pointers
  u16 mode;
  int result;
  size_t rem_length = *length;

  // request format: TOGGLE cust_id mode END
  // split_message should turn this into:
  // TOGGLE
  // cust_id
  // mode = 0 or 1
  // END

  word_count = split_message(request, words, expected_words);

  if (word_count != expected_words) {
    trace_printk("L4.5 ALERT: toggle word count error = %d\n", word_count);
    // we did not get a valid message, so return error message
    goto error_msg;
  }

  result = kstrtou16((words[1]), 10, &cust_id);
  if (result != 0) {
    trace_printk(
      "L4.5 ALERT: toggle cust_id %s, error = %d\n", words[1], result);
    // cust id string to u16 failed, so return error message
    goto error_msg;
  }

  result = kstrtou16((words[2]), 10, &mode);
  if (result != 0) {
    trace_printk("L4.5 ALERT: toggle mode %s, error = %d\n", words[2], result);
    // cust id string to u16 failed, so return error message
    goto error_msg;
  }

  message = json_objOpen(message, NULL, &rem_length);
  message = json_uint(message, "ID", cust_id, &rem_length);

  result = toggle_customization_active_mode(cust_id, mode);

  message = json_uint(message, "Result", result, &rem_length);

  message = json_objClose(message, &rem_length);
  message = json_end_message(message, &rem_length);

  *length = *length - rem_length;

  return;

// TODO: Messages and lengths should be variables
error_msg:
  message = json_objOpen(message, NULL, &rem_length);
  message = json_nstr(message, "ERROR", "parsing", 7, &rem_length);
  message = json_objClose(message, &rem_length);
  message = json_end_message(message, &rem_length);
  *length = *length - rem_length;
  return;
}

void
netlink_set_cust_priority(char* message, size_t* length, char* request)
{
  u16 cust_id = 0;
  int word_count = 0;
  int expected_words = 4;
  char* words[4]; // hold expected_words pointers to char pointers
  u16 priority;
  int result;
  size_t rem_length = *length;

  // request format: PRIORITY cust_id priority END
  // split_message should turn this into:
  // PRIORITY
  // cust_id
  // priority
  // END

  word_count = split_message(request, words, expected_words);

  if (word_count != expected_words) {
    trace_printk("L4.5 ALERT: priority word count error = %d\n", word_count);
    // we did not get a valid message, so return error message
    goto error_msg;
  }

  result = kstrtou16((words[1]), 10, &cust_id);
  if (result != 0) {
    trace_printk(
      "L4.5 ALERT: priority cust_id %s, error = %d\n", words[1], result);
    // cust id string to u16 failed, so return error message
    goto error_msg;
  }

  result = kstrtou16((words[2]), 10, &priority);
  if (result != 0) {
    trace_printk("L4.5 ALERT: priority %s, error = %d\n", words[2], result);
    // cust id string to u16 failed, so return error message
    goto error_msg;
  }

  message = json_objOpen(message, NULL, &rem_length);
  message = json_uint(message, "ID", cust_id, &rem_length);

  result = set_customization_priority(cust_id, priority);

  message = json_uint(message, "Result", result, &rem_length);

  message = json_objClose(message, &rem_length);
  message = json_end_message(message, &rem_length);

  *length = *length - rem_length;

  return;

// TODO: Messages and lengths should be variables
error_msg:
  message = json_objOpen(message, NULL, &rem_length);
  message = json_nstr(message, "ERROR", "parsing", 7, &rem_length);
  message = json_objClose(message, &rem_length);
  message = json_end_message(message, &rem_length);
  *length = *length - rem_length;
  return;
}