#!/bin/sh
# Usual directory for downloading software in ProtoGENI hosts is `/local`
cd /local

GIT_DIR=/users/dflukasz/software_defined_customization
DCA_KERNEL_DIR=$GIT_DIR/DCA_kernel
SIMPLE_SERVER_DIR=$GIT_DIR/experiment_scripts/client_server

##### Check if file is there #####
if [ ! -f "./layer4_5_installed.txt" ]
then
    #### Create the file ####
    sudo touch "./layer4_5_installed.txt"

    #### Run  one-time commands ####

    #Install necessary packages
    sudo apt-get update & EPID=$!
    wait $EPID
    sudo apt install -y sshpass curl iperf3 net-tools & EPID=$!
    wait $EPID

    # Install custom software
    cd /users/dflukasz
    sudo git clone https://github.com/danluke2/software_defined_customization.git & EPID=$!
    wait $EPID
    sudo chown -R dflukasz /users/dflukasz/software_defined_customization


    LINE=8
    FILE=$GIT_DIR/config.sh
    sudo sed -i "${LINE}d" $FILE
    sudo sed -i "${LINE}i\GIT_DIR=$GIT_DIR" $FILE

    cd $GIT_DIR
    sudo ./config.sh

    sudo $DCA_KERNEL_DIR/bash/installer.sh & EPID=$!
    wait $EPID

    sudo depmod


fi
##### Run Boot-time commands
# Start my service -- assume it was installed at /usr/local/bin
cd /users/dflukasz/software_defined_customization
sudo git pull
