#!/usr/bin/env bash

# make sure apt is up to date before installer runs
apt update

# give system a little time to sync with ntp
# need to grep for inactive here to reset service if necessary
if timedatectl | grep -q "inactive"; then
    systemctl restart systemd-timesyncd.service
fi

echo "******** Sleeping 20 seconds to allow ntp sync **********"
sleep 20

timedatectl

# ************** STANDARD PARAMS MUST GO HERE ****************
GIT_DIR=/home/vagrant/software_defined_customization
DCA_KERNEL_DIR=/home/vagrant/software_defined_customization/DCA_kernel
SIMPLE_SERVER_DIR=/home/vagrant/software_defined_customization/experiment_scripts/client_server
# ************** END STANDARD PARAMS ****************

#replace dnsmasq config to match experiments
cp $GIT_DIR/vagrant/dnsmasq.conf /etc/dnsmasq.conf

# Update .bashrc to include some aliases
cat <<EOT >>/home/vagrant/.bashrc
alias edit='sudo gedit ~/.bashrc'
alias src='source ~/.bashrc'
alias desk='cd /home/vagrant/Desktop'
alias modules='cd /usr/lib/modules/$(uname -r)/'

alias tracelog='sudo gedit /sys/kernel/tracing/trace'
alias cyclelog="sudo trace-cmd clear && sudo bash -c 'echo 1 > /sys/kernel/tracing/tracing_on'"

alias installer="clear && sudo $DCA_KERNEL_DIR/bash/installer.sh && sudo bash -c 'echo 1 > /sys/kernel/tracing/tracing_on'"

alias server_echo='python3 $SIMPLE_SERVER_DIR/echo_server.py'
alias client_echo='python3 $SIMPLE_SERVER_DIR/echo_client.py'

alias clean_layer='sudo rm -rf /usr/lib/modules/$(uname -r)/layer4_5'

tracecopy () {
    sudo cp /sys/kernel/tracing/trace $GIT_DIR/\$1
    sudo trace-cmd clear
    sudo bash -c 'echo 1 > /sys/kernel/tracing/tracing_on'
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

# Ubuntu 22.04 version (future use)
# cat <<EOT >>/etc/netplan/50-vagrant.yaml
#       routes:
#       - to: 10.0.0.0/24
#         via: 10.0.0.20
#       nameservers:
#           search: [mydomain, otherdomain]
#           addresses: [10.0.0.20, 8.8.8.8]
# EOT

netplan apply

#turn swap memory back on
swapon -a

# finish with Layer 4.5 install script
$DCA_KERNEL_DIR/bash/installer.sh

# ********* BASE Ubuntu 20.04 VM Settings ***************

#lock kernel version so we don't need to deal with updates for testing
# sudo apt-mark hold linux-image-generic-hwe-20.04
# sudo apt-mark hold linux-generic-hwe-20.04
# sudo apt-mark hold linux-headers-generic-hwe-20.04

# this should stop excessive dns queries
# dpkg --remove whoopsie
# systemctl disable avahi-daemon
# systemctl stop avahi-daemon

# apt install trace-cmd
# apt -y remove unattended-upgrades

# timedatectl set-ntp on

# guest additions included
# add-apt-repository -y ppa:wireshark-dev/stable
# add-apt-repository -y ppa:linuxgndu/sqlitebrowser
# apt update
# apt install -y git sshpass curl dnsmasq sqlitebrowser wireshark openssh-server iperf3 net-tools
# usermod -a -G wireshark vagrant
# sudo pip install matplotlib #so avail for all users
# echo "PermitRootLogin yes" >> /etc/ssh/sshd_config
# systemctl restart ssh
# sudo pip install pycryptodome # for python encryption support in NCO
