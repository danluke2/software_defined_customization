# Software Defined Network Customization at Layer 4.5


Prototype of Layer 4.5 customization framework to match NetSoft 2022 submission paper (under review).  Contains a Network-wide Customization Orchestrator (NCO) to distribute Layer 4.5 customization modules to devices.  NCO communicates with Device Customization Agent (DCA) to deliver the module (DCA\_user).  The DCA\_kernel code will handle the registration of the customization module and inserting the module into the socket flow between the socket layer and transport layer.


Acronyms:

1) NCO: Network-wide Customization Orchestrator

1) DCA: Device Customization Agent

1) CIB: Customization Information Base


## Overview:

<a href="url"><img src="https://github.com/danluke2/software_defined_customization/blob/main/assets/stack.png" align="center" height="400"  ></a>


1) NCO distributes customizations to devices over a control channel for insertion at Layer 4.5

    * Layer 4.5 is transparent to application and transport layers


<a href="url"><img src="https://github.com/danluke2/software_defined_customization/blob/main/assets/nco_host.png" align="center" height="400"  ></a>


2) NCO has several internal components to support distribution and management of the deployed customization

    * Construct: responsible for building the per-device customization module to include embedding necessary parameters and storing all values in the CIB

    * Deploy: supports transport of customization modules, in binary format, to devices on the networ

    * Revoke: support the removal of outdated or misbehaving customization modules from a customized device

    * Monitor: allows for retrieving module use statistics across the network to aid in forensics analysis

    * Security: provide a mechanism for adding per-network module security requirements to match a given threat model

    * Middlebox: interface with network controlled middlebox device to allow processing a deployed customization



<a href="url"><img src="https://github.com/danluke2/software_defined_customization/blob/main/assets/automation.png" align="center" height="400"  ></a>


3) DCA establishes the control channel with NCO to manage customizations installed on the device.

    * DCA_user establishes the control channel with NCO

    * DCA_kernel encompasses Layer 4.5 logic to manage customizations on the device

3) Customization modules register with Layer 4.5 DCA

    * register the protocol (TCP or UDP), application (task) name, destination port, destination IPv4 address, source IPv4 address (if server), and source port (if desired) for tracking sockets

        * server knows the source and dest IP since it binds to a source IP

        * clients don't know source IP since IP table lookup has not happened yet

        * source port (client) generally not useful since randomly assigned value

        * destination port (server) generally not useful since randomly assigned by the client

    * provide send\_function and recv\_function pointers to be stored and
    applied to customized sockets

        * the send or recv function may also be NULL if not customizing that path

        * if both are NULL, then customization is rejected


### Youtube Videos:

  * NetVerify 21 presentation focused on Layer 4.5 and initial idea of network wide control (15 min): https://youtu.be/s9vwJLDMSlI?start=17737&end=18730


<!--
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

    * after all sockets have customization removed, then allow customization module to be unloaded -->


<!-- ## Prerequisites:

1) Vagrant: https://www.vagrantup.com

1) VirtualBox: https://www.virtualbox.org

1) Approximately 20GB of hard drive space

    * 5GB Vagrant box

    * 11GB VMDK for server VM

    * 11GB VMDK for client VM


1) If not using Vagrant VM:

    * Download and configure Ubuntu 20.04+ running kernel 5.11+

    * View notes in Vagrantfile and setup.sh on how to configure VM for experiments and install Layer 4.5 module


1) If changes to common variables are desired:

    * Update config.sh with new value to existing variable

    * Execute config.sh to update all necessary files to reflect the new value


## Vagrant VM settings:

  * username/password: vagrant/vagrant

  * username/password: root/vagrant

  * VBOX specific:

      * base memory: 4096

      * 2 CPU

      * video: 32MB

      * Network 1: NAT, Paravirtualized adapter

  * various aliases inserted by setup.sh script -->




## Layer 4.5 using Vagrant and VirtualBox:

#### Prerequisites:

1) Vagrant: https://www.vagrantup.com

1) VirtualBox: https://www.virtualbox.org

1) Approximately 30GB of hard drive space

    * 5GB Vagrant box

    * 11GB VMDK for server VM

    * 11GB VMDK for client VM



#### Install Steps

1) Git clone this repo

    * create your own branch after cloning to avoid accidental changes to master:

        * `git checkout -b someName`



1) (Optional) If changes to common variables are desired:

    * Update config.sh with new value to existing variable

    * Execute config.sh to update all necessary files to reflect the new value


1) (Optional) Update Makefile to reflect desired debug level:

    * software_defined_customization/DCA\_kernel/Makefile

    * Makefile is set with DEBUG defined by default to print messages to a trace file

1) Navigate to the vagrant folder and install base machine:

    * `cd software_defined_customization/vagrant`

    * (Optional) Edit the included Vagrantfile with desired parameters

        * a different vagrant box can be used, but must be configured with dependencies manually

        * If desired, turn off the virtualbox GUI by commenting out vg.gui = true

        * adjust the allotted memory if your machine does not have sufficient RAM to support 2 VM's with 8GB each

        * setup.sh: shell script to run on first install (provisioning)

    * `vagrant up`  

        * If you get 'bandwidthctl' error, then comment out that line in the vagrant file and run up command again


1) Wait for machine to download and install Layer 4.5 and dependencies

    * NOTE: BTF error is not an issue at the moment and will be remedied later


1) Use the GUI or SSH into VM and check install:

    * `vagrant ssh server` (client)

    * `lsmod | grep layer`

    * NOTE: location of layer4_5 module: /usr/lib/modules/$(uname -r)/layer4_5

    * NOTE: Layer 4.5 will auto load at startup and any modules present in the new module customizations folder will load after checking if Layer 4.5 is running


#### Vagrant VM settings:

  * username/password: vagrant/vagrant

  * username/password: root/vagrant

  * VBOX specific:

      * base memory: 8192

      * 2 CPU

      * video: 32MB

      * Network 1: NAT, Paravirtualized adapter

      * Network 2: Internal Network, Paravirtualized adapter, 1Gbps link speed

  * various aliases inserted by setup.sh script


## Layer 4.5 on your own Ubuntu VM:

NOTE: Other Linux OS's are possible, but you need to adjust scripts to reflect your environment

1) Download and configure Ubuntu 20.04+ running kernel 5.11+

    * View notes in Vagrantfile and setup.sh on how to configure VM for experiments

1) Install layer 4.5 kernel module

    * `cd software_defined_customization/DCA_kernel/bash`

    * `sudo ./installer.sh`






## Next Steps:

1) To get experience with Layer 4.5 modules and how they are used, Use the README provided in the sample\_modules folder to run the provided sample modules

    * Sample modules don't use the NCO or user-space DCA component and focus on Layer 4.5 only

1) To run experiments from paper, see README in experiment\_scripts and modules in experiment\_modules
