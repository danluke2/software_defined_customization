#!/bin/bash

#Purpose: #Purpose: cleans up files from experiments
#$1 is section to run




# ************** STANDARD PARAMS MUST GO HERE ****************
GIT_DIR=/home/vagrant/software_defined_customization
NCO_DIR=/home/vagrant/software_defined_customization/NCO
EXP_SCRIPT_DIR=/home/vagrant/software_defined_customization/experiment_scripts
EXP_MOD_DIR=/home/vagrant/software_defined_customization/experiment_modules
SIMPLE_SERVER_DIR=/home/vagrant/software_defined_customization/experiment_scripts/client_server
DCA_KERNEL_DIR=/home/vagrant/software_defined_customization/DCA_kernel
DCA_USER_DIR=/home/vagrant/software_defined_customization/DCA_user

SERVER_IP=10.0.0.20
SERVER_PASSWD=vagrant
CLIENT_IP=10.0.0.40
CLIENT_PASSWD=vagrant




# ************** STANDARD PARAMS MUST GO HERE ****************

WIRESHARK_DIR=/usr/lib/x86_64-linux-gnu/wireshark/plugins

# NCO/DCA
if [ "$1" = "NCO" ]; then
  cd $EXP_SCRIPT_DIR
  rm nco_deploy.png
  cd logs
  rm n*
  cd
  rm -rf device_modules
  rm cib.db
fi



# Bulk
if [ "$1" = "BULK" ]; then
  cd $EXP_SCRIPT_DIR
  rm bulk_overhead.png
  cd logs
  rm b*
  cd $EXP_MOD_DIR
  make clean
fi


# Batch
if [ "$1" = "BATCH" ]; then
  cd $EXP_SCRIPT_DIR
  rm batch_overhead.png
  cd logs
  rm b*
  cd $EXP_MOD_DIR
  make clean
fi


# Challenge
if [ "$1" = "CHALLENGE" ]; then
  cd $NCO_DIR
  rm cib.db
  cd device_modules
  rm -rf h*
fi


# Middlebox
if [ "$1" = "MIDDLEBOX" ]; then
  cd $NCO_DIR
  rm cib.db
  cd device_modules
  rm -rf h*
  cd $GIT_DIR
  rm middle_demo.pcap
  cd $WIRESHARK_DIR
  sudo rm demo*
fi
