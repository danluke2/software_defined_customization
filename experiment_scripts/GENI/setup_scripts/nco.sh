#!/bin/sh
# Usual directory for downloading software in ProtoGENI hosts is `/local`
cd /local

GIT_DIR=/users/dflukasz/software_defined_customization
NCO_DIR=$GIT_DIR/NCO
GENI_DIR=$GIT_DIR/experiment_scripts/GENI

##### Check if file is there #####
if [ ! -f "./nco_installed.txt" ]
then
    #### Create the file ####
    sudo touch "./nco_installed.txt"

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
    sudo chown -R dflukasz $GIT_DIR


    LINE=8
    FILE=$GIT_DIR/config.sh
    sudo sed -i "${LINE}d" $FILE
    sudo sed -i "${LINE}i\GIT_DIR=$GIT_DIR" $FILE

    cd $GIT_DIR
    sudo ./config.sh

    NCO_DIR=/
    sudo cp $GENI_DIR/setup_scripts/nco.service etc/systemd/system/nco.service

    #register loader with systemd
    sudo systemctl enable nco.service

fi
##### Run Boot-time commands
# Start my service -- assume it was installed at /usr/local/bin
cd /users/dflukasz/software_defined_customization
sudo git pull
sudo systemctl restart nco.service
