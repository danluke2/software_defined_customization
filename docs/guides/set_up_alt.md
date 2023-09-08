---
layout: post
title: Layer 4.5 Alternate Environments
parent: Guides
permalink: guides/set_up_alt
nav_order: 2
---
### Alternative Installation Options ###


## Layer 4.5 on your own Ubuntu VM:

NOTE: Other Linux OS's are possible, but you need to adjust scripts to reflect your environment (mainly the installer file)

1) Download and configure Ubuntu 20.04+ running kernel 5.11+

- View notes in [Vagrantfile] and [setup.sh] on how to configure VM for experiments

1) Install layer 4.5 kernel module

```bash
cd software_defined_customization/DCA_kernel/bash
```
```bash
sudo ./installer.sh
```

[Vagrantfile]: https://github.com/danluke2/software_defined_customization/blob/main/vagrant/Vagrantfile
[setup.sh]: https://github.com/danluke2/software_defined_customization/blob/main/vagrant/setup.sh