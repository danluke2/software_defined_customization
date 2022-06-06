# Layer 4.5 Module Requirements

# Purpose:
Set the requirements for module development.  Not properly handling memory in
modules can easily result in a crashed kernel.

### RULES:

1) Modules must register a valid send and recv function pointer.  If one path is not desired to customize, then use the no_cust flag to skip the customization in that function.

1) Do not free the modules send or recv buffer, this is handled by Layer 4.5

1) If copy_length is set to 0, then no data will be delivered to the application
or layer 4.  This allows for dropping data. (Needs some testing)

1) if copy_length is < 0, this signals an error has occurred

1) Setting the no_cust flag on the buffer allows skipping the customization for this round

1) Setting set_cust_to_skip will bypass all future customization for this socket (send and recv)

1) I do not recommend using memcpy when dealing with the src_iter buffer.  This
buffer is not guaranteed to be linear and you may alter memory that is fragile.
Instead use kernel provided methods when dealing with the src_iter structure.  
These include:

    * copy_from_iter_full: which guarantees a full copy is performed or fails
    and reverts to the previous state

    * iov_iter_advance: adjusts the iter buffer X bytes and properly sets
    all relevant values of the structure

    * iov_iter_revert: adjust iter buffer X bytes backwards

1) Do not modify the src_iter structure given to the module, unless done as side
effect of standard function (copy_from_iter_full)


1) to autoload a module on reboot, put the .ko file in /usr/lib/modules/$(uname -r)/layer4_5/customizations folder

1) Modules now have a activated setting to allow a module to attach to a socket, but not customize the traffic if active_mode is set to false.  This setting can be toggled via the NCO/DCA.

1) A deprecated module will still customize a socket that it was previously attached to when it became deprecated.  The module will not attach to new sockets once it has transitioned to a deprecated state.

1) pid and tgid may have different task names associated with them.  tgid likely holds the application name you want, such as `dig`