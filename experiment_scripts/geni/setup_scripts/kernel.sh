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

fi
##### Run Boot-time commands
