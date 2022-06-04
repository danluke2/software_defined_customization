#!/bin/bash

#Purpose:  Sets the config values across files in this repo
# Default values will correspond to vagrant settings
# find start and end param lines, delete all lines between them, and then insert new params

distro=$(uname -r)

# ******************** UPDATE SECTION  ************************

# the base directory that the git project will be put into
GIT_DIR=/home/vagrant/software_defined_customization

# GIT repo structure
NCO_DIR=$GIT_DIR/NCO

EXP_SCRIPT_DIR=$GIT_DIR/experiment_scripts
BUFFER_SCRIPT_DIR=$EXP_SCRIPT_DIR/buffered
SIMPLE_SERVER_DIR=$EXP_SCRIPT_DIR/client_server

LAYER_MOD_DIR=$GIT_DIR/layer4_5_modules
BUFFER_MOD_DIR=$LAYER_MOD_DIR/buffering

DCA_KERNEL_DIR=$GIT_DIR/DCA_kernel
DCA_USER_DIR=$GIT_DIR/DCA_user

# Installer/loader specific variables
INSTALLER_MAKEFILE_DIR=$DCA_KERNEL_DIR
INSTALL_LOCATION=/usr/lib/modules/$distro/layer4_5
INCLUDE_DIR=/usr/lib/modules/$distro/build/include
# location where customization modules must exist to be loaded at runtime
CUST_LOCATION=$INSTALL_LOCATION/customizations
DCA_LOCATION=$INSTALL_LOCATION/DCA

# Makefile Parameters
DISTRO_DIR=/lib/modules/$distro
#only in module makefile
KBUILD_EXTRA_SYMBOLS=$DISTRO_DIR/layer4_5/Module.symvers

# VAGRANT VM machine settings
SERVER_IP=10.0.0.20
SERVER_PASSWD="vagrant"
CLIENT_IP=10.0.0.40
CLIENT_PASSWD="vagrant"

# ******************** UPDATE SECTION  ************************

# ----------------------------------------------------------------
# Function for delete lines between start params and end params
#   Accepts 1 argument:
#     File name
# ----------------------------------------------------------------
delete_lines() {
  START="$(grep -n "STANDARD PARAMS MUST GO HERE" $1 | head -n 1 | cut -d: -f1)"
  END="$(grep -n "END STANDARD PARAMS" $1 | head -n 1 | cut -d: -f1)"

  # delete all lines between start and end
  ((START = START + 1))
  ((END = END - 1))
  if [ $START -lt $END ]; then
    sed -i "${START},${END}d" $1
  fi
}

# *************** Installer Makefile update ***************

FILE=$INSTALLER_MAKEFILE_DIR/Makefile

# delete all lines between start and end
delete_lines $FILE

# insert standard params
LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
((LINE = LINE + 1))
sed -i "${LINE}i\INSTALLER_MAKEFILE_DIR=$INSTALLER_MAKEFILE_DIR" $FILE
((LINE = LINE + 1))
sed -i "${LINE}i\INSTALL_LOCATION=$INSTALL_LOCATION" $FILE
((LINE = LINE + 1))
sed -i "${LINE}i\DISTRO_DIR=$DISTRO_DIR" $FILE

# *************** Installer Makefile update ***************

# *************** Other Makefile updates ***************

MAKEFILE_PATHS="$LAYER_MOD_DIR/sample_modules  $BUFFER_MOD_DIR"

for x in $MAKEFILE_PATHS; do
  FILE=$x/Makefile
  delete_lines $FILE

  # insert standard params
  LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
  ((LINE = LINE + 1))
  sed -i "${LINE}i\KBUILD_EXTRA_SYMBOLS=$KBUILD_EXTRA_SYMBOLS" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\MODULE_DIR=$x" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\DISTRO_DIR=$DISTRO_DIR" $FILE
done

# *************** Other Makefile updates ***************

# *************** Installer Bash updates ***************

INSTALLER_FILES="installer.sh loader.sh"
for x in $INSTALLER_FILES; do
  FILE=$INSTALLER_MAKEFILE_DIR/bash/${x}
  delete_lines $FILE

  # insert standard params
  LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
  ((LINE = LINE + 1))
  sed -i "${LINE}i\INSTALLER_MAKEFILE_DIR=$INSTALLER_MAKEFILE_DIR" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\INSTALL_LOCATION=$INSTALL_LOCATION" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\INCLUDE_DIR=$INCLUDE_DIR" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\CUST_LOCATION=$CUST_LOCATION" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\GIT_DIR=$GIT_DIR" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\DCA_LOCATION=$DCA_LOCATION" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\DCA_USER_DIR=$DCA_USER_DIR" $FILE
done

# *************** Installer Bash updates ***************

# *************** NCO Bash updates ***************

INSTALLER_FILES="service.sh"
for x in $INSTALLER_FILES; do
  FILE=$NCO_DIR/$x
  delete_lines $FILE

  # insert standard params
  LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
  ((LINE = LINE + 1))
  sed -i "${LINE}i\NCO_DIR=$NCO_DIR" $FILE
done

# *************** DCA_user Bash updates ***************

# *************** Python simple server Bash update ***************

INSTALLER_FILES="service.sh"
for x in $INSTALLER_FILES; do
  FILE=$SIMPLE_SERVER_DIR/$x
  delete_lines $FILE

  # insert standard params
  LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
  ((LINE = LINE + 1))
  sed -i "${LINE}i\SIMPLE_SERVER_DIR=$SIMPLE_SERVER_DIR" $FILE
done

# *************** DCA_user Bash updates ***************

# *************** Vagrant Bash updates ***************

FILE=$GIT_DIR/vagrant/setup.sh
delete_lines $FILE

# insert standard params
LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
((LINE = LINE + 1))
sed -i "${LINE}i\GIT_DIR=$GIT_DIR" $FILE
((LINE = LINE + 1))
sed -i "${LINE}i\DCA_KERNEL_DIR=$DCA_KERNEL_DIR" $FILE
((LINE = LINE + 1))
sed -i "${LINE}i\SIMPLE_SERVER_DIR=$SIMPLE_SERVER_DIR" $FILE

# *************** Installer Bash updates ***************

# *************** Buffer Bash updates ***************

# list of files to modify
FILES="batch_experiment_buffering.sh bulk_experiment_buffering.sh bulk_experiment_buffering_tls.sh cleanup.sh"

for x in $FILES; do
  FILE=$BUFFER_SCRIPT_DIR/${x}
  delete_lines $FILE

  # insert standard params
  LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
  ((LINE = LINE + 1))
  sed -i "${LINE}i\GIT_DIR=$GIT_DIR" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\NCO_DIR=$NCO_DIR" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\EXP_SCRIPT_DIR=$EXP_SCRIPT_DIR" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\BUFFER_SCRIPT_DIR=$BUFFER_SCRIPT_DIR" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\LAYER_MOD_DIR=$LAYER_MOD_DIR" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\BUFFER_MOD_DIR=$BUFFER_MOD_DIR" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\SIMPLE_SERVER_DIR=$SIMPLE_SERVER_DIR" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\DCA_KERNEL_DIR=$DCA_KERNEL_DIR" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\DCA_USER_DIR=$DCA_USER_DIR" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\CUST_LOCATION=$CUST_LOCATION" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\SERVER_IP=$SERVER_IP" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\SERVER_PASSWD=$SERVER_PASSWD" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\CLIENT_IP=$CLIENT_IP" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\CLIENT_PASSWD=$CLIENT_PASSWD" $FILE
done

# *************** Buffer Bash updates ***************

# *************** NCO Builder update ***************

FILE=$NCO_DIR/builder.sh
delete_lines $FILE

# insert standard params
LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
((LINE = LINE + 1))
sed -i "${LINE}i\NCO_DIR=$NCO_DIR" $FILE
((LINE = LINE + 1))
sed -i "${LINE}i\NCO_MOD_DIR=$NCO_MOD_DIR" $FILE

# *************** NCO Builder update ***************

# *************** NCO cfg update ***************

FILE=$NCO_DIR/cfg.py
delete_lines $FILE

# insert standard params
LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
((LINE = LINE + 1))
sed -i "${LINE}i\HOST='$SERVER_IP'" $FILE
((LINE = LINE + 1))
sed -i "${LINE}i\nco_dir='$NCO_DIR/'" $FILE
((LINE = LINE + 1))
sed -i "${LINE}i\nco_mod_dir='$NCO_MOD_DIR/'" $FILE
((LINE = LINE + 1))
sed -i "${LINE}i\common_struct_dir='$DCA_KERNEL_DIR/'" $FILE

# *************** NCO cfg update ***************

# *************** DCA cfg update ***************

FILE=$DCA_USER_DIR/cfg.py
delete_lines $FILE

# insert standard params
LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
((LINE = LINE + 1))
sed -i "${LINE}i\HOST='$SERVER_IP'" $FILE
((LINE = LINE + 1))
sed -i "${LINE}i\dca_dir='$DCA_USER_DIR/'" $FILE
((LINE = LINE + 1))
sed -i "${LINE}i\symver_location='$INSTALL_LOCATION/'" $FILE
((LINE = LINE + 1))
sed -i "${LINE}i\system_release='$distro'" $FILE

# *************** DCA cfg update ***************

# *************** Module Include updates ***************

# sample modules
SAMPLE_MODULE_FILES="buffer_python_client.c active_buffer_python_server.c buffer_python_server.c"
for x in $SAMPLE_MODULE_FILES; do
  FILE=$LAYER_MOD_DIR/sample_modules/${x}
  delete_lines $FILE

  # insert standard params
  LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
  ((LINE = LINE + 1))
  sed -i "${LINE}i\#include <common_structs.h>" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\#include <printing.h>" $FILE
done

# buffer modules
BUFFER_MODULE_FILES="overhead_test_batch_dns_client.c overhead_test_batch_dns_server_buffer.c overhead_test_bulk_file_client_buffer.c overhead_test_bulk_file_server.c overhead_test_bulk_file_tls_client.c overhead_test_bulk_file_tls_server.c"
for x in $BUFFER_MODULE_FILES; do
  FILE=$BUFFER_MOD_DIR/${x}
  delete_lines $FILE

  # insert standard params
  LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
  ((LINE = LINE + 1))
  sed -i "${LINE}i\#include <common_structs.h>" $FILE
  ((LINE = LINE + 1))
  sed -i "${LINE}i\#include <printing.h>" $FILE
done
