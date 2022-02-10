# Software Defined Network Customization at Layer 4.5

# Purpose:
Prototype of Layer 4.5 customization framework to match HotNet 2022 submission paper (under review).  Contains a Netwrok-wide Customization Orchestrator (NCO)
to distribute Layer 4.5 customization modules to devices.  NCO communicates with Device Customization Agent (DCA) to deliver the module (DCA\_user).  The DCA\_kernel code will handle the registration of the customization module and inserting the module into the socket flow between the socket layer and transport layer.

## Acronyms:

1) NCO: Network-wide Customization Orchestrator

1) DCA: Device Customization Agent


### Status:


1) Customization modules register with Layer 4.5 DCA

    * register the protocol (TCP or UDP), application (task) name, destination port,
    destination IPv4 address, source IPv4 address (if server), and source port (if desired)
    for tracking sockets

        * server knows the source and dest IP since it binds to a source IP

        * clients don't know source IP since IP table lookup has not happened yet

        * source port generally not useful since randomly assigned value

    * provide send\_function and recv\_function pointers to be stored and
    applied to customized sockets

        * the send or recv function may also be NULL if not customizing that path

        * if both are NULL, then customization is rejected




2) tap.c: intercepts TCP and UDP calls, creates a customizable socket with
key derived Sock FD

    * socket creation encompasses customization assignment based on registered
    customization nodes

    * if socket will be customized, send/recv customization functions are
    stored in customization\_socket struct

    * each socket can have a single customization applied


1) On send/recv, tap.c returns a call to DCA (if customizing socket), which in
turn will report the number of bytes sent to the app, as if modifications did not take place

    * send: DCA calls stored send_function to modify the buffer  

    * recv: DCA calls stored recv_function to modify the buffer




4) Customization modules unregister with Layer 4.5 DCA when no longer needed:

    * remove customization from available list so can't be applied to new sockets

    * remove customization from all active sockets using it

    * after all sockets have customization removed, then allow customization module to be unloaded



# Steps to install Layer 4.5:

1) Install Ubuntu 20.04.3 LTS (Focal Fossa) VM (tested on kernel 5.13.0-28)

    * default install includes python3

1) Install Wireshark (Optional):

    * sudo add-apt-repository ppa:wireshark-dev/stable

        * if that fails, try running 'sudo apt upgrade' first

    * sudo apt update

    * sudo apt install wireshark

        * select 'yes' to allow non-root users to capture packets

    * sudo adduser $USER wireshark

    * restart VM

1) Install git on VM: sudo apt install git

1) Git clone this repo

    * create your own branch after cloning to avoid accidental changes to master: git checkout -b someName

1) Run install.sh script with sudo privileges: sudo software\_defined\_customization/DCA\_kernel/bash/installer

    * this should install all required dependencies (otherwise submit issue)

    * adjust the layer 4.5 Makefile to compile in the desired debug level

        * DEBUG - useful messages to see various events, such as customization applied to socket

        * DEBUG1 - extra events that may happen but also are not very useful in general

        * DEBUG2 - events that may happen often and situationally helpful

        * DEBUG3 - events that would generate a lot of messages

1) Verify no errors during install and that layer4_5 kernel module is inserted: lsmod \| grep layer

    * location of layer4_5 modules: /usr/lib/modules/$(uname -r)/layer4_5

    * Layer 4.5 will auto load at startup and any modules present in the new module
    customizations folder will load after checking if Layer 4.5 is running


# Steps to run sample python3 customization:

1) In the test\_modules folder, make and install sample python kernel modules:

    * Modify Makefile in this folder to adjust path variables

        * KBUILD_EXTRA_SYMBOLS must point to layer 4.5 generated Module.symvers file

        * MODULE_DIR points to directory holding modules to be built

        * BUILD\_MODULE is command line arg to direct building a specific module

    * run 'make BUILD\_MODULE=sample\_python\_client.o' and 'make BUILD\_MODULE=sample\_python\_server.o'

        * verify no errors during module build

    * insert both module:

        * sudo insmod sample\_python\_client.ko

        * sudo insmod sample\_python\_server.ko

        * if DEBUG enabled, verify modules loading messages present in trace log

            * /sys/kernel/tracing/trace

    * to autoload on reboot, put the .ko file in /usr/lib/modules/$(uname -r)/layer4_5/customizations folder

1) In new terminal window, launch tcpdump to verify changes are applied to messages:

    * sudo tcpdump -i any -X

    * alternatively, launch Wireshark

1) Launch the python echo client and server in two separate terminals:

    * /test\_scripts/client\_server

    * python3 echo\_server.py --proto tcp (udp)

    * python3 echo\_client.py --proto tcp (udp)

1) If desired to help read trace log file, in another terminal, grep for process id's
 of target customization: pgrep python3

1) type some messages into the echo client and verify tcpdump shows modified messages

    * echo client/server should show un-modified messages if both sides customized

1) type 'quit' to close client connection, which may also terminate the server
(otherwise terminate the server)

1) dump the kernel trace file to find corresponding messages for layer 4.5 messages:

    * sudo gedit /sys/kernel/tracing/trace

    * adjust layer 4.5 Makefile if more debug messages are desired

    * reset the trace file between runs if desired (as root, sudo su): > /sys/kernel/tracing/trace
