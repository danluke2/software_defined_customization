#!/bin/bash

#Purpose: perform the NCO/DCA module deployment experiment
# $1 = number of trials
# $2 = client IP address for customization modules

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

# Force root
if [[ "$(id -u)" != "0" ]]; then
  echo "This script must be run as root" 1>&2
  exit -1
fi

# 130.127.215.157 = clemson
# 192.12.245.165 = colorado
# 128.206.119.40 = missouri
# 128.171.8.123 = hawaii
# 128.95.190.54 = washington
# 192.122.236.116 = cornell

# 198.82.156.38 = virginia tech
# 72.36.65.68 = illinois
# 165.124.51.195 = northwestern
# 192.86.139.70 = nyu -> works
# 140.254.14.100 = ohio state
# 129.110.253.30 = UT

# first batch
for server in 130.127.215.157 192.12.245.165 128.206.119.40 128.171.8.123 128.95.190.54 192.122.236.116; do
  echo "*************** Performing Bulk File Experiment with Server $server ***************"
  $GENI_SCRIPT_DIR/geni_bulk_single_experiment.sh $1 $server $2
done

# second batch
# for server in 128.112.170.33 198.82.156.38 72.36.65.68 165.124.51.195 192.86.139.70 140.254.14.100 129.110.253.30; do
#   echo "*************** Performing Bulk File Experiment with Server $server ***************"
#   $GENI_SCRIPT_DIR/geni_bulk_single_experiment.sh $2 $server $4
# done
