#!/bin/bash

#Purpose: perform dns requests repeatedly to test immediate cust attachment
# $1 = number of trials
# $2 = server to connect to

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
USERNAME=
PASSWORD=
# ************** END STANDARD PARAMS  ****************

SERVER_IP=$2

# Each trial is about 75 seconds long
for ((i = 1; i <= $1; i++)); do
    for ((j = 1; j <= 15; j++)); do
        query="www.test_$i$j.com"
        dig +time=5 +tries=2 @$SERVER_IP -p 53 $query
        sleep 5
    done
done
