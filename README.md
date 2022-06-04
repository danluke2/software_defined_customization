# Software Defined Network Customization at Layer 4.5


Prototype of Layer 4.5 customization framework to match accepted NetSoft 2022 paper, but now using a different Layer 4.5 model that allows application message buffering.  

Contains a Network-wide Customization Orchestrator (NCO) to distribute Layer 4.5 customization modules to devices.  NCO communicates with Device Customization Agent (DCA) to deliver the module (DCA\_user).  The DCA\_kernel code will handle the registration of the customization module and inserting the module into the socket flow between the socket layer and transport layer.


Acronyms:

1) NCO: Network-wide Customization Orchestrator

1) DCA: Device Customization Agent

1) CIB: Customization Information Base


Refer the master branch README for additional details on architecture.




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

    * switch to buffering branch

        * `git branch -v -a`

        * `git switch -c buffering origin/buffering`

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






## Next Steps:

1) To get experience with Layer 4.5 modules and how they are used, Use the README provided in the layer4_5_modules/sample\_modules folder to run the provided sample modules

    * Sample modules don't use the NCO or user-space DCA component and focus on Layer 4.5 only

1) To run the provided experiments, see README in experiment\_scripts/buffered and modules in layer4_5_modules/buffering
