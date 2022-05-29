#!/usr/bin/env bash

#lock kernel version so we don't need to deal with updates for testing
sudo apt-mark hold linux-image-generic-hwe-20.04
sudo apt-mark hold linux-generic-hwe-20.04
sudo apt-mark hold linux-headers-generic-hwe-20.04

# this should stop excessive dns queries
dpkg --remove whoopsie
systemctl stop avahi-daemon
systemctl disable avahi-daemon

apt update
apt -y remove unattended-upgrades

timedatectl set-ntp on
timedatectl

# ************** STANDARD PARAMS MUST GO HERE ****************
GIT_DIR=/home/vagrant/software_defined_customization
DCA_KERNEL_DIR=/home/vagrant/software_defined_customization/DCA_kernel
SIMPLE_SERVER_DIR=/home/vagrant/software_defined_customization/experiment_scripts/client_server
DCA_KERNEL_DIR=/home/vagrant/software_defined_customization/DCA_kernel
SIMPLE_SERVER_DIR=/home/vagrant/software_defined_customization/experiment_scripts/client_server

# ************** STANDARD PARAMS MUST GO HERE ****************

$DCA_KERNEL_DIR/bash/installer.sh

#replace dnsmasq config to match experiments
cp $GIT_DIR/vagrant/dnsmasq.conf /etc/dnsmasq.conf

# Update .bashrc to include some aliases
cat <<EOT >>/home/vagrant/.bashrc
alias edit='sudo gedit ~/.bashrc'
alias src='source ~/.bashrc'
alias desk='cd /home/vagrant/Desktop'
alias modules='cd /usr/lib/modules/$(uname -r)/'

alias tracelog='sudo gedit /sys/kernel/tracing/trace'
alias cyclelog="sudo bash -c '> /sys/kernel/tracing/trace'"

alias installer='clear && sudo $DCA_KERNEL_DIR/bash/installer.sh'

alias server_echo='python3 $SIMPLE_SERVER_DIR/echo_server.py'
alias client_echo='python3 $SIMPLE_SERVER_DIR/echo_client.py'

alias clean_layer='sudo rm -rf /usr/lib/modules/$(uname -r)/layer4_5'

tracecopy () {
    sudo cp /sys/kernel/tracing/trace $GIT_DIR/\$1
    sudo bash -c '> /sys/kernel/tracing/trace'
}
EOT

# allow scripting ssh commands
touch /home/vagrant/.ssh/config
cat <<EOT >>/home/vagrant/.ssh/config
Host 10.0.0.20
    StrictHostKeyChecking no

Host 10.0.0.40
    StrictHostKeyChecking no
EOT

# fix network interface GW and DNS server
cat <<EOT >>/etc/netplan/50-vagrant.yaml
      gateway4: 10.0.0.20
      nameservers:
          search: [mydomain, otherdomain]
          addresses: [10.0.0.20, 8.8.8.8]
EOT

netplan apply
