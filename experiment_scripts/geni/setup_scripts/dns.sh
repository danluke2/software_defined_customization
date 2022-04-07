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
if [ ! -f "./dns_installed.txt" ]
then
    #### Create the file ####
    sudo touch "./dns_installed.txt"

    #### Run  one-time commands ####

    #Install necessary packages
    sudo apt-get update & EPID=$!
    wait $EPID
    sudo apt install -y sshpass curl dnsmasq iperf3 net-tools & EPID=$!
    wait $EPID

    # Install custom software
    cd /users/$GENI_USERNAME
    sudo git clone https://github.com/danluke2/software_defined_customization.git & EPID=$!
    wait $EPID
    sudo chown -R $GENI_USERNAME $GIT_DIR


    LINE=14
    FILE=$GIT_DIR/config.sh
    sudo sed -i "${LINE}d" $FILE
    sudo sed -i "${LINE}i\GIT_DIR=$GIT_DIR" $FILE

    # Update NCO IP address
    LINE=49
    FILE=$GIT_DIR/config.sh
    sudo sed -i "${LINE}d" $FILE
    sudo sed -i "${LINE}i\SERVER_IP=10.10.0.5" $FILE


    cd $GIT_DIR
    sudo ./config.sh

    sudo $DCA_KERNEL_DIR/bash/installer.sh & EPID=$!
    wait $EPID


    #replace dnsmasq config to match experiments
    sudo cp $GENI_SCRIPT_DIR/dnsmasq.conf /etc/dnsmasq.conf


fi
##### Run Boot-time commands
# Start my service -- assume it was installed at /usr/local/bin
cd $GIT_DIR
sudo git pull

sleep 10

cd $DCA_USER_DIR
sudo su $GENI_USERNAME -c 'sudo python3 DCA.py --iface eth1 --logging &'

sudo systemctl stop systemd-resolved.service

sudo systemctl enable dnsmasq.service

sudo systemctl restart dnsmasq.service
