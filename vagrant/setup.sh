#!/usr/bin/env bash

apt update
apt -y upgrade

timedatectl set-ntp on
timedatectl

# install Layer 4.5 and auto load on boot
GIT_DIR=/home/vagrant/software_defined_customization

cd $GIT_DIR

DCA_kernel/bash/installer.sh $GIT_DIR/DCA_kernel

depmod


#replace dnsmasq config to match experiments
cp $GIT_DIR/vagrant/dnsmasq.conf /etc/dnsmasq.conf

# Update .bashrc to include some aliases
cat <<EOT >> /home/vagrant/.bashrc
alias edit='sudo gedit ~/.bashrc'
alias src='source ~/.bashrc'
alias desk='cd /home/vagrant/Desktop'
alias modules='cd /usr/lib/modules/$(uname -r)/'

alias tracelog='sudo gedit /sys/kernel/tracing/trace'
alias cyclelog="sudo bash -c '> /sys/kernel/tracing/trace'"

alias installer='clear && sudo /home/vagrant/software_defined_customization/DCA_kernel/bash/installer.sh'

alias server_echo='python3 /home/vagrant/software_defined_customization/experiment_scripts/client_server/echo_server.py'
alias client_echo='python3 /home/vagrant/software_defined_customization/experiment_scripts/client_server/echo_client.py'

alias cleanup='cd /home/vagrant/software_defined_customization/sample_modules && make clean && cd ../experiment_modules && make clean && cd device_modules && rm -rf h* && cd /home/vagrant/software_defined_customization/DCA_kernel && make clean && cd test_modules && make clean && cd /home/vagrant/software_defined_customization/NCO/device_modules && rm -rf h* && cd /home/vagrant/software_defined_customization/experiment_scripts/logs && rm b* && rm n* && cd .. && rm *.png'

alias clean_layer='sudo rm -rf /usr/lib/modules/$(uname -r)/layer4_5'
EOT



# allow scripting ssh commands
touch /home/vagrant/.ssh/config
cat <<EOT >> /home/vagrant/.ssh/config
Host 10.0.0.20
    StrictHostKeyChecking no

Host 10.0.0.40
    StrictHostKeyChecking no
EOT



# fix network interface GW and DNS server
cat <<EOT >> /etc/netplan/50-vagrant.yaml
      gateway4: 10.0.0.20
      nameservers:
          search: [mydomain, otherdomain]
          addresses: [10.0.0.20, 8.8.8.8]
EOT

netplan apply
