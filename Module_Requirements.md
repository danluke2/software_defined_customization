# Layer 4.5 Module Requirements

# Purpose:
Set the requirements for module development.  Not properly handling memory in
modules can easily result in a crashed kernel.

### RULES:

1) If the modules send or recv buffer is freed and set to NULL, then no future
customization calls will occur for that module

1) (TODO) If copy_length is set to 0, then no data will be delivered to the application
or layer 4.  This allows for dropping data.

1) (TODO) if copy_length is < 0, this signals that no copy from cust buffer to src_iter
is requested; effectively allowing cust module to behave like a passive module

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
