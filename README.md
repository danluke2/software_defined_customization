# Software Defined Network Customization at Layer 4.5


Prototype of Layer 4.5 customization framework to match NetSoft 2022 submission paper (under review).  Contains a Network-wide Customization Orchestrator (NCO)
to distribute Layer 4.5 customization modules to devices.  NCO communicates with Device Customization Agent (DCA) to deliver the module (DCA\_user).  The DCA\_kernel code will handle the registration of the customization module and inserting the module into the socket flow between the socket layer and transport layer.


Acronyms:

1) NCO: Network-wide Customization Orchestrator

1) DCA: Device Customization Agent

1) CIB: Customization Information Base



## Overview:


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



## Layer 4.5 VM Location:

1) Download the pre-configured Layer 4.5 Ubuntu Focal Fossa VM: TBD



## Steps to manually install Layer 4.5:

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

    * Makefile is set with DEBUG defined by default to print messages to a trace file


1) Verify no errors during install and that layer4_5 kernel module is inserted: lsmod \| grep layer

    * NOTE: BTF error is not an issue at the moment and will be remedied later

    * location of layer4_5 modules: /usr/lib/modules/$(uname -r)/layer4_5

    * Layer 4.5 will auto load at startup and any modules present in the new module
    customizations folder will load after checking if Layer 4.5 is running



## Next Steps:

1) To run sample modules, see README in sample\_modules

1) To run experiments from paper, see README in test\_scripts and modules in test\_modules
