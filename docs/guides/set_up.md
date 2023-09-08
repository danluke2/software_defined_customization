---
layout: post
title: Layer 4.5 with Vagrant and VirtualBox
parent: Guides
permalink: guides/set_up
nav_order: 1
---
## Layer 4.5 using Vagrant and VirtualBox:

#### Prerequisites:

1) [Vagrant]

2) [VirtualBox]

3) Approximately 30GB of hard drive space

- 5GB Vagrant box

-  11GB VMDK for server VM

-  11GB VMDK for client VM



#### Install Steps

1) Git clone this repo

-  create your own branch after cloning to avoid accidental changes to master:

```bash
git checkout -b someName
```

2) If changes to common variables or file paths are required/desired:

-  Update [config.sh] with new value to existing variable

    -  Only need to update portion marked 'UPDATE SECTION'

-  Execute [config.sh] to update all necessary files to reflect the new value


3) (Optional) Update [Makefile] to reflect desired debug level:

-  [Makefile] is set with DEBUG defined by default to print messages to a trace file


4) Navigate to the vagrant folder and install base machine:

 - ```bash
cd software_defined_customization/vagrant
```

-  (Optional) Edit the included [Vagrantfile] with desired parameters

    -  a different vagrant box can be used, but must be configured with dependencies manually

    -  If desired, turn off the virtualbox GUI by commenting out both instances of the below lines in [Vagrantfile]:

        `vg.gui = true`

    -  adjust the allotted memory if your machine does not have sufficient RAM to support 2 VM's with 8GB each

    -  [setup.sh] : shell script to run on first install (provisioning)

 - ```bash
vagrant up  
```

    -  If you get 'bandwidthctl' error, then comment out both instances of the below lines in [Vagrantfile]:

        `vb.customize ["bandwidthctl", :id, "add", "VagrantLimit", "--type", "network", "--limit", "1000m"]`

        `vb.customize ["modifyvm", :id, "--nicbandwidthgroup2", "VagrantLimit"]`


5) Wait for machine to download and install Layer 4.5 and dependencies

-  NOTE: BTF error is not an issue at the moment and will be remedied later


6) Use the GUI or SSH into VM and check install:

(client)

-  ```bash
vagrant ssh server
``` 

-  ```bash
lsmod | grep layer
```

-  NOTE: location of layer4_5 module: ` /usr/lib/modules/$(uname -r)/layer4_5 `

-  NOTE: Layer 4.5 will auto load at startup and any modules present in the new module customizations folder will load after checking if Layer 4.5 is running


#### Vagrant VM settings:

  - username/password: vagrant/vagrant

  - username/password: root/vagrant

  - VBOX specific:

    -  base memory: 8192

    -  2 CPU

    -  video: 32MB

    -  Network 1: NAT, Paravirtualized adapter

    -  Network 2: Internal Network, Paravirtualized adapter, 1Gbps link speed

  - various aliases inserted by [setup.sh] script


[Vagrant]: https://www.vagrantup.com
[VirtualBox]: https://www.virtualbox.org
[config.sh]: https://github.com/danluke2/software_defined_customization/blob/main/config.sh
[Makefile]: https://github.com/danluke2/software_defined_customization/blob/main/DCA_kernel/Makefile
[Vagrantfile]: https://github.com/danluke2/software_defined_customization/blob/main/vagrant/Vagrantfile
[setup.sh]: https://github.com/danluke2/software_defined_customization/blob/main/vagrant/setup.sh