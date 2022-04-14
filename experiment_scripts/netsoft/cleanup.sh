#!/bin/bash

#Purpose: #Purpose: cleans up files from experiments
#$1 is section to run




# ************** STANDARD PARAMS MUST GO HERE ****************
GIT_DIR=/home/vagrant/software_defined_customization
NCO_DIR=/home/vagrant/software_defined_customization/NCO
EXP_SCRIPT_DIR=/home/vagrant/software_defined_customization/experiment_scripts
NETSOFT_SCRIPT_DIR=/home/vagrant/software_defined_customization/experiment_scripts/netsoft
GENI_SCRIPT_DIR=/home/vagrant/software_defined_customization/experiment_scripts/geni
LAYER_MOD_DIR=/home/vagrant/software_defined_customization/layer4_5_modules
NETSOFT_MOD_DIR=/home/vagrant/software_defined_customization/layer4_5_modules/netsoft
GENI_MOD_DIR=/home/vagrant/software_defined_customization/layer4_5_modules/geni
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
  cd $NETSOFT_SCRIPT_DIR
  rm nco_deploy.png
  cd $EXP_SCRIPT_DIR/logs
  rm nco_results_*.txt
  cd
  rm -rf device_modules
  rm cib.db
fi



# Bulk
if [ "$1" = "BULK" ]; then
  cd $NETSOFT_SCRIPT_DIR
  rm bulk_overhead.png
  cd $EXP_SCRIPT_DIR/logs
  rm bulk_*.txt
  cd $NETSOFT_MOD_DIR
  make clean
fi


# Batch
if [ "$1" = "BATCH" ]; then
  cd $NETSOFT_SCRIPT_DIR
  rm batch_overhead.png
  cd $EXP_SCRIPT_DIR/logs
  rm batch_*.txt
  cd $NETSOFT_MOD_DIR
  make clean
fi


# Challenge
if [ "$1" = "CHALLENGE" ]; then
  cd $NCO_DIR
  rm cib.db
  cd device_modules
  rm -rf host*
fi


# Middlebox
if [ "$1" = "MIDDLEBOX" ]; then
  cd $DCA_USER_DIR
  rm *_messages.log
  cd $NCO_DIR
  rm cib.db
  rm nco_messages.log
  cd device_modules
  rm -rf host*
  cd $GIT_DIR
  rm middle_demo.pcap
  cd $WIRESHARK_DIR
  sudo rm demo*.lua
fi
