#!/bin/bash

#Purpose:  Sets the config values across files in this repo
# Default values will correspond to vagrant settings

distro=$(uname -r)
#directory holding the software_defined_customization git repo
GIT_DIR=/home/vagrant/software_defined_customization


# Installer/loader specific variables
INSTALLER_MAKEFILE_DIR=$GIT_DIR/DCA_kernel
INSTALL_LOCATION=/usr/lib/modules/$distro/layer4_5
# location where customization modules must exist to be loaded at runtime
CUST_LOCATION=$INSTALL_LOCATION/customizations


# Makefile Parameters
DISTRO_DIR=/lib/modules/$distro
#only in module makefile
KBUILD_EXTRA_SYMBOLS=$DISTRO_DIR/layer4_5/Module.symvers


# Git directory structures for bash scripts
NCO_DIR=$GIT_DIR/NCO
EXP_SCRIPT_DIR=$GIT_DIR/experiment_scripts
EXP_MOD_DIR=$GIT_DIR/experiment_modules
DCA_KERNEL_DIR=$GIT_DIR/DCA_kernel
DCA_USER_DIR=$GIT_DIR/DCA_user
SIMPLE_SERVER_DIR=$EXP_SCRIPT_DIR/client_server


# VM machine settings
SERVER_IP=10.0.0.20
SERVER_PASSWD="vagrant"
CLIENT_IP=10.0.0.40
CLIENT_PASSWD="vagrant"



# *************** Installer Makefile update ***************
# file line to start writing at
LINE=2

FILE=$INSTALLER_MAKEFILE_DIR/Makefile
sed -i "${LINE}d" $FILE
sed -i "${LINE}i\INSTALLER_MAKEFILE_DIR=$INSTALLER_MAKEFILE_DIR" $FILE
((LINE=LINE+1))
sed -i "${LINE}d" $FILE
sed -i "${LINE}i\INSTALL_LOCATION=$INSTALL_LOCATION" $FILE
((LINE=LINE+1))
sed -i "${LINE}d" $FILE
sed -i "${LINE}i\DISTRO_DIR=$DISTRO_DIR" $FILE

# *************** Installer Makefile update ***************



# *************** Other Makefile updates ***************

MAKEFILE_PATHS="$DCA_KERNEL_DIR/test_modules $EXP_MOD_DIR $EXP_MOD_DIR/geni $GIT_DIR/sample_modules "

for x in $MAKEFILE_PATHS
do
  LINE=2 # file line to start writing at
  FILE=$x/Makefile
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\KBUILD_EXTRA_SYMBOLS=$KBUILD_EXTRA_SYMBOLS" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\MODULE_DIR=$x" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\DISTRO_DIR=$DISTRO_DIR" $FILE
done

# *************** Other Makefile updates ***************



# *************** Installer Bash updates ***************

INSTALLER_FILES="installer.sh loader.sh"
for x in $INSTALLER_FILES
do
  LINE=10 # file line to start writing at
  FILE=$INSTALLER_MAKEFILE_DIR/bash/${x}
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\INSTALLER_MAKEFILE_DIR=$INSTALLER_MAKEFILE_DIR" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\INSTALL_LOCATION=$INSTALL_LOCATION" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\CUST_LOCATION=$CUST_LOCATION" $FILE
done

# *************** Installer Bash updates ***************



# *************** Vagrant Bash updates ***************

FILE=$GIT_DIR/vagrant/setup.sh
LINE=10 # file line to start writing at

sed -i "${LINE}d" $FILE
sed -i "${LINE}i\GIT_DIR=$GIT_DIR" $FILE
((LINE=LINE+1))
sed -i "${LINE}d" $FILE
sed -i "${LINE}i\DCA_KERNEL_DIR=$DCA_KERNEL_DIR" $FILE
((LINE=LINE+1))
sed -i "${LINE}d" $FILE
sed -i "${LINE}i\SIMPLE_SERVER_DIR=$SIMPLE_SERVER_DIR" $FILE


# *************** Installer Bash updates ***************



# *************** Other Bash updates ***************

# list of files to modify
FILES="batch_experiment.sh bulk_experiment.sh challenge_response.sh cleanup.sh exp_builder.sh middle_demo.sh nco_dca_batch_experiment.sh nco_dca_experiment.sh"

for x in $FILES
do
  LINE=10 # file line to start writing at
  FILE=$EXP_SCRIPT_DIR/${x}
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\GIT_DIR=$GIT_DIR" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\NCO_DIR=$NCO_DIR" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\EXP_SCRIPT_DIR=$EXP_SCRIPT_DIR" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\EXP_MOD_DIR=$EXP_MOD_DIR" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\SIMPLE_SERVER_DIR=$SIMPLE_SERVER_DIR" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\DCA_KERNEL_DIR=$DCA_KERNEL_DIR" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\DCA_USER_DIR=$DCA_USER_DIR" $FILE
  ((LINE=LINE+1))
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\SERVER_IP=$SERVER_IP" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\SERVER_PASSWD=$SERVER_PASSWD" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\CLIENT_IP=$CLIENT_IP" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\CLIENT_PASSWD=$CLIENT_PASSWD" $FILE

done

# *************** Other Bash updates ***************



# *************** NCO Builder update ***************
# file line to start writing at
LINE=12

FILE=$NCO_DIR/builder.sh
sed -i "${LINE}d" $FILE
sed -i "${LINE}i\NCO_DIR=$NCO_DIR" $FILE

# *************** NCO Builder update ***************



# *************** NCO cfg update ***************
# file line to start writing at
LINE=2

FILE=$NCO_DIR/cfg.py
sed -i "${LINE}d" $FILE
sed -i "${LINE}i\HOST='$SERVER_IP'" $FILE
((LINE=LINE+1))
sed -i "${LINE}d" $FILE
sed -i "${LINE}i\nco_dir='$NCO_DIR/'" $FILE
# *************** NCO Builder update ***************
