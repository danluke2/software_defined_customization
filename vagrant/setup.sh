#!/bin/bash

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


# finish with Layer 4.5 install script
$DCA_KERNEL_DIR/bash/installer.sh
