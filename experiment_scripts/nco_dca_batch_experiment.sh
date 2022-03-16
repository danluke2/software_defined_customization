#!/bin/bash

#Purpose: perform the NCO/DCA module deployment experiment
# $1 = number of trials
# $2 = number of hosts



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


# NOTE: reverse this order to speed up tests
# NOTE2: if reversing, can also remove --construct flag after the 250 test is
# completed to avoid re-making each module
for hosts in 10 50 100 175 250
do
  echo "*************** Performing Experiment with $hosts Hosts ***************"
  $EXP_SCRIPT_DIR/nco_dca_experiment.sh $1 $hosts $GIT_DIR $EXP_SCRIPT_DIR
done


echo generating plot

python3 nco_dca_plot.py
