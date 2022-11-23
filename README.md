# Software Defined Network Customization at Layer 4.5


Prototype of Layer 4.5 customization framework to match NetSoft 2022 paper titled "Towards Software Defined Layer 4.5 Customization".  Layer 4.5 contains a Network-wide Customization Orchestrator (NCO) to distribute Layer 4.5 customization modules to devices.  The NCO communicates with a Device Customization Agent (DCA) to deliver the module (DCA\_user).  The DCA\_kernel code will handle the registration of the customization module and inserting the module into the socket flow between the socket layer and transport layer.


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



1) If changes to common variables or file paths are required/desired:

    * Update config.sh with new value to existing variable

        * Only need to update portion marked 'UPDATE SECTION'

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

NOTE: Other Linux OS's are possible, but you need to adjust scripts to reflect your environment (mainly the installer file)

1) Download and configure Ubuntu 20.04+ running kernel 5.11+

    * View notes in Vagrantfile and setup.sh on how to configure VM for experiments

1) Install layer 4.5 kernel module

    * `cd software_defined_customization/DCA_kernel/bash`

    * `sudo ./installer.sh`



## Branches
1) Buffering: introduces a different approach for receive side processing to allow the customization module to buffer L4 data for the application.  This basically allows processing for stricter applications, such as those that use TLS.

1) Rotating: introduces the capability to rotate customization modules on an active socket and maintain backward compatibility until both end points have same customization module active (i.e., due to transmission delays).



## Next Steps:

1) To get experience with Layer 4.5 modules and how they are used, Use the README provided in the layer4_5_modules/sample\_modules folder to run the provided sample modules

    * Sample modules don't use the NCO or user-space DCA component and focus on Layer 4.5 only

1) To run experiments from paper, see README in experiment\_scripts/netsoft and modules in layer4_5_modules/netsoft
