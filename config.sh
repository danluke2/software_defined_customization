#!/bin/bash

#Purpose:  Sets the config values across files in this repo
# Default values will correspond to vagrant settings


#directory holding the software_defined_customization git repo
GIT_DIR=/home/vagrant/software_defined_customization
NCO_DIR=$GIT_DIR/NCO
EXP_SCRIPT_DIR=$GIT_DIR/experiment_scripts
EXP_MOD_DIR=$GIT_DIR/experiment_modules
SIMPLE_SERVER_DIR=$EXP_SCRIPT_DIR/client_server
DCA_KERNEL_DIR=$GIT_DIR/DCA_kernel
DCA_USER_DIR=$GIT_DIR/DCA_user

SERVER_IP=10.0.0.20
SERVER_PASSWD="vagrant"
CLIENT_IP=10.0.0.40
CLIENT_PASSWD="vagrant"

LINE=10

FILES="batch_experiment.sh bulk_experiment.sh challenge_response.sh exp_builder.sh middle_demo.sh nca_dca_batch_experiment.sh nco_dca_experiment.sh"

for file in $FILES
do
  sed -i "${LINE}d" $EXP_SCRIPT_DIR/${FILES}
  sed -i "${LINE}i\GIT_DIR=$GIT_DIR" $EXP_SCRIPT_DIR/${FILES}
  ((LINE=LINE+1))
  sed -i "${LINE}d" $EXP_SCRIPT_DIR/${FILES}
  sed -i "${LINE}i\NCO_DIR=$NCO_DIR" $EXP_SCRIPT_DIR/${FILES}
  ((LINE=LINE+1))
  sed -i "${LINE}d" $EXP_SCRIPT_DIR/${FILES}
  sed -i "${LINE}i\EXP_SCRIPT_DIR=$EXP_SCRIPT_DIR" $EXP_SCRIPT_DIR/${FILES}
  ((LINE=LINE+1))
  sed -i "${LINE}d" $EXP_SCRIPT_DIR/${FILES}
  sed -i "${LINE}i\EXP_MOD_DIR=$EXP_MOD_DIR" $EXP_SCRIPT_DIR/${FILES}
  ((LINE=LINE+1))
  sed -i "${LINE}d" $EXP_SCRIPT_DIR/${FILES}
  sed -i "${LINE}i\SIMPLE_SERVER_DIR=$SIMPLE_SERVER_DIR" $EXP_SCRIPT_DIR/${FILES}
  ((LINE=LINE+1))
  sed -i "${LINE}d" $EXP_SCRIPT_DIR/${FILES}
  sed -i "${LINE}i\DCA_KERNEL_DIR=$DCA_KERNEL_DIR" $EXP_SCRIPT_DIR/${FILES}
  ((LINE=LINE+1))
  sed -i "${LINE}d" $EXP_SCRIPT_DIR/${FILES}
  sed -i "${LINE}i\DCA_USER_DIR=$DCA_USER_DIR" $EXP_SCRIPT_DIR/${FILES}
  ((LINE=LINE+1))
  ((LINE=LINE+1))
  sed -i "${LINE}d" $EXP_SCRIPT_DIR/${FILES}
  sed -i "${LINE}i\SERVER_IP=$SERVER_IP" $EXP_SCRIPT_DIR/${FILES}
  ((LINE=LINE+1))
  sed -i "${LINE}d" $EXP_SCRIPT_DIR/${FILES}
  sed -i "${LINE}i\SERVER_PASSWD=$SERVER_PASSWD" $EXP_SCRIPT_DIR/${FILES}
  ((LINE=LINE+1))
  sed -i "${LINE}d" $EXP_SCRIPT_DIR/${FILES}
  sed -i "${LINE}i\CLIENT_IP=$CLIENT_IP" $EXP_SCRIPT_DIR/${FILES}
  ((LINE=LINE+1))
  sed -i "${LINE}d" $EXP_SCRIPT_DIR/${FILES}
  sed -i "${LINE}i\CLIENT_PASSWD=$CLIENT_PASSWD" $EXP_SCRIPT_DIR/${FILES}
  ((LINE=LINE+1))


done
