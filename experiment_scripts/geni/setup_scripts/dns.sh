#!/bin/sh
# Usual directory for downloading software in ProtoGENI hosts is `/local`
cd /local

# ************** STANDARD PARAMS MUST GO HERE ****************
GENI_USERNAME=$1

GIT_DIR=/users/$GENI_USERNAME/software_defined_customization
NCO_DIR=$GIT_DIR/NCO
DCA_KERNEL_DIR=$GIT_DIR/DCA_kernel
DCA_USER_DIR=$GIT_DIR/DCA_user
EXP_SCRIPT_DIR=$GIT_DIR/experiment_scripts
SIMPLE_SERVER_DIR=$EXP_SCRIPT_DIR/client_server
GENI_SCRIPT_DIR=$EXP_SCRIPT_DIR/geni

# ************** STANDARD PARAMS MUST GO HERE ****************

##### Check if file is there #####
if [ ! -f "./kernel_update.txt" ]; then
    #### Create the file ####
    sudo touch "./kernel_update.txt"

    #### Run  one-time commands ####
    sudo apt-get update &
    EPID=$!
    wait $EPID

    sudo apt install -y linux-image-5.4.0-122-generic &
    EPID=$!
    wait $EPID

    sudo reboot &
    EPID=$!
    wait $EPID

    #just trying to stop it from reaching next part of code
    sudo apt-get update &
    EPID=$!
    wait $EPID
    sudo apt install -y sshpass curl dnsmasq iperf3 net-tools trace-cmd &
    EPID=$!
    wait $EPID

fi

if [ ! -f "./dns_installed.txt" ]; then

    #### Run  one-time commands ####

    #lock kernel version so we don't need to deal with updates for testing
    sudo apt-mark hold linux-image-generic
    sudo apt-mark hold linux-generic
    sudo apt-mark hold linux-headers-generic

    #Install necessary packages
    sudo apt-get update &
    EPID=$!
    wait $EPID
    sudo apt install -y sshpass curl dnsmasq iperf3 net-tools trace-cmd &
    EPID=$!
    wait $EPID

    # Install custom software
    cd /users/$GENI_USERNAME
    sudo git clone -b rotating https://github.com/danluke2/software_defined_customization.git &
    EPID=$!
    wait $EPID
    sudo chown -R $GENI_USERNAME $GIT_DIR

    FILE=$GIT_DIR/config.sh
    LINE="$(grep -n "GIT_DIR=" $FILE | head -n 1 | cut -d: -f1)"
    sudo sed -i "${LINE}d" $FILE
    sudo sed -i "${LINE}i\GIT_DIR=$GIT_DIR" $FILE

    # Update NCO IP address
    LINE="$(grep -n "SERVER_IP=" $FILE | head -n 1 | cut -d: -f1)"
    sudo sed -i "${LINE}d" $FILE
    sudo sed -i "${LINE}i\SERVER_IP=10.10.0.5" $FILE

    cd $GIT_DIR
    sudo ./config.sh

    sudo $DCA_KERNEL_DIR/bash/installer.sh &
    EPID=$!
    wait $EPID

    #replace dnsmasq config to match experiments
    sudo cp $GENI_SCRIPT_DIR/dnsmasq.conf /etc/dnsmasq.conf

    #### Create the file ####
    sudo touch "./dns_installed.txt"

fi

##### Run Boot-time commands
# Start my service -- assume it was installed at /usr/local/bin
cd $GIT_DIR
sudo git pull

sleep 10

cd $DCA_USER_DIR
sudo su $GENI_USERNAME -c 'sudo python3 DCA.py --iface eth1 &'

sudo systemctl stop systemd-resolved.service

sudo systemctl enable dnsmasq.service

sudo systemctl restart dnsmasq.service
