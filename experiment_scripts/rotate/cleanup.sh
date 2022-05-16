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
CUST_LOCATION=/usr/lib/modules/5.13.0-35-generic/layer4_5/customizations

SERVER_IP=10.0.0.20
SERVER_PASSWD=vagrant
CLIENT_IP=10.0.0.40
CLIENT_PASSWD=vagrant

# ************** STANDARD PARAMS MUST GO HERE ****************

WIRESHARK_DIR=/usr/lib/x86_64-linux-gnu/wireshark/plugins

# NCO/DCA
if [ "$1" = "DEPRECATE" ]; then
  cd $EXP_SCRIPT_DIR/logs
  rm deprecate_exp.txt
  cd $NCO_DIR
  rm cib.db
  cd device_modules
  rm -rf host*
fi


# Bypass and ApplyNow experiment
if [ "$1" = "BYPASS" ]; then
  cd $EXP_SCRIPT_DIR/logs
  rm bypass_exp.txt
  cd $NCO_DIR
  rm cib.db
  cd device_modules
  rm -rf host*
fi




# Challenge
if [ "$1" = "CHALLENGE" ]; then
  cd $NCO_DIR
  rm cib.db
  cd device_modules
  rm -rf host*

fi


# Remove NCO and DCA logs
if [ "$1" = "LOGS" ]; then
  cd $NCO_DIR
  rm nco_messages.log
  cd $DCA_USER_DIR
  rm *messages.log
fi