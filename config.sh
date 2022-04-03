#!/bin/bash

#Purpose:  Sets the config values across files in this repo
# Default values will correspond to vagrant settings

# $1 = geni username
# $2 = geni password

distro=$(uname -r)

# ******************** UPDATE SECTION  ************************

# the base directory that the git project will be put into
GIT_DIR=/home/vagrant/software_defined_customization


# GIT repo structure
NCO_DIR=$GIT_DIR/NCO

EXP_SCRIPT_DIR=$GIT_DIR/experiment_scripts
NETSOFT_SCRIPT_DIR=$EXP_SCRIPT_DIR/netsoft
GENI_SCRIPT_DIR=$EXP_SCRIPT_DIR/geni
SIMPLE_SERVER_DIR=$EXP_SCRIPT_DIR/client_server

LAYER_MOD_DIR=$GIT_DIR/layer4_5_modules
NETSOFT_MOD_DIR=$LAYER_MOD_DIR/netsoft
GENI_MOD_DIR=$LAYER_MOD_DIR/geni

DCA_KERNEL_DIR=$GIT_DIR/DCA_kernel
DCA_USER_DIR=$GIT_DIR/DCA_user


# Installer/loader specific variables
INSTALLER_MAKEFILE_DIR=$DCA_KERNEL_DIR
INSTALL_LOCATION=/usr/lib/modules/$distro/layer4_5
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

GENI_USERNAME=$1
GENI_PASSWORD=$2

# ******************** UPDATE SECTION  ************************


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

MAKEFILE_PATHS="$DCA_KERNEL_DIR/test_modules $LAYER_MOD_DIR/sample_modules  $GENI_MOD_DIR $NETSOFT_MOD_DIR"

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
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\DCA_LOCATION=$DCA_LOCATION" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\DCA_USER_DIR=$DCA_USER_DIR" $FILE
done

# *************** Installer Bash updates ***************




# *************** DCA_user Bash updates ***************

INSTALLER_FILES="service.sh"
for x in $INSTALLER_FILES
do
  LINE=10 # file line to start writing at
  FILE=$DCA_USER_DIR/$x
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\DCA_LOCATION=$DCA_LOCATION" $FILE
done

# *************** DCA_user Bash updates ***************


# *************** NCO Bash updates ***************

INSTALLER_FILES="service.sh"
for x in $INSTALLER_FILES
do
  LINE=10 # file line to start writing at
  FILE=$NCO_DIR/$x
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\NCO_DIR=$NCO_DIR" $FILE
done

# *************** DCA_user Bash updates ***************


# *************** Python simple server Bash update ***************

INSTALLER_FILES="service.sh"
for x in $INSTALLER_FILES
do
  LINE=10 # file line to start writing at
  FILE=$SIMPLE_SERVER_DIR/$x
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\SIMPLE_SERVER_DIR=$SIMPLE_SERVER_DIR" $FILE
done

# *************** DCA_user Bash updates ***************


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



# *************** NetSoft Bash updates ***************

# list of files to modify
FILES="batch_experiment.sh bulk_experiment.sh challenge_response.sh cleanup.sh exp_builder.sh middle_demo.sh nco_dca_batch_experiment.sh nco_dca_experiment.sh"

for x in $FILES
do
  LINE=10 # file line to start writing at
  FILE=$NETSOFT_SCRIPT_DIR/${x}
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
  sed -i "${LINE}i\NETSOFT_SCRIPT_DIR=$NETSOFT_SCRIPT_DIR" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\GENI_SCRIPT_DIR=$GENI_SCRIPT_DIR" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\LAYER_MOD_DIR=$LAYER_MOD_DIR" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\NETSOFT_MOD_DIR=$NETSOFT_MOD_DIR" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\GENI_MOD_DIR=$GENI_MOD_DIR" $FILE
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

# *************** NetSoft Bash updates ***************



# *************** GENI Bash updates ***************

# list of files to modify
FILES="geni_batch_all_servers.sh geni_batch_single_experiment.sh geni_bulk_single_experiment.sh geni_public_dns_single_experiment.sh"

for x in $FILES
do
  LINE=12 # file line to start writing at
  FILE=$GENI_SCRIPT_DIR/${x}
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
  sed -i "${LINE}i\NETSOFT_SCRIPT_DIR=$NETSOFT_SCRIPT_DIR" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\GENI_SCRIPT_DIR=$GENI_SCRIPT_DIR" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\LAYER_MOD_DIR=$LAYER_MOD_DIR" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\NETSOFT_MOD_DIR=$NETSOFT_MOD_DIR" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\GENI_MOD_DIR=$GENI_MOD_DIR" $FILE
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
  sed -i "${LINE}i\USERNAME=$GENI_USERNAME" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\PASSWORD=$GENI_PASSWORD" $FILE
done


# *************** GENI Bash updates ***************




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
# *************** NCO cfg update ***************


# *************** DCA cfg update ***************
# file line to start writing at
LINE=2

FILE=$DCA_USER_DIR/cfg.py
sed -i "${LINE}d" $FILE
sed -i "${LINE}i\HOST='$SERVER_IP'" $FILE
((LINE=LINE+1))
sed -i "${LINE}d" $FILE
sed -i "${LINE}i\dca_dir='$DCA_USER_DIR/'" $FILE
((LINE=LINE+1))
sed -i "${LINE}d" $FILE
sed -i "${LINE}i\symver_location='$INSTALL_LOCATION/'" $FILE
((LINE=LINE+1))
sed -i "${LINE}d" $FILE
sed -i "${LINE}i\system_release='$distro'" $FILE
# *************** DCA cfg update ***************




# *************** Module Include updates ***************

# sample modules
SAMPLE_MODULE_FILES="sample_python_client.c sample_python_server.c"
for x in $SAMPLE_MODULE_FILES
do
  LINE=12 # file line to start writing at
  FILE=$LAYER_MOD_DIR/sample_modules/${x}
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\#include \"$DCA_KERNEL_DIR/common_structs.h\"" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\#include \"$DCA_KERNEL_DIR/util/printing.h\"" $FILE
done

# netsoft modules
NETSOFT_MODULE_FILES="overhead_test_batch_dns_client.c overhead_test_batch_dns_server.c overhead_test_bulk_file_client.c overhead_test_bulk_file_server.c"
for x in $NETSOFT_MODULE_FILES
do
  LINE=12 # file line to start writing at
  FILE=$NETSOFT_MOD_DIR/${x}
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\#include \"$DCA_KERNEL_DIR/common_structs.h\"" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\#include \"$DCA_KERNEL_DIR/util/printing.h\"" $FILE
done


# geni modules
GENI_MODULE_FILES="bulk_client.c bulk_server.c compress_dns_client.c compress_dns_server.c end_dns_client.c end_dns_server.c front_dns_client.c front_dns_server.c middle_dns_client.c middle_dns_server.c"
for x in $GENI_MODULE_FILES
do
  LINE=13 # file line to start writing at
  FILE=$GENI_MOD_DIR/${x}
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\#include \"$DCA_KERNEL_DIR/common_structs.h\"" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\#include \"$DCA_KERNEL_DIR/util/printing.h\"" $FILE
done


# extra modules
EXTRA_MODULE_FILES="sample_tls_client.c sample_tls_server.c"
for x in $EXTRA_MODULE_FILES
do
  LINE=12 # file line to start writing at
  FILE=$LAYER_MOD_DIR/extra/${x}
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\#include \"$DCA_KERNEL_DIR/common_structs.h\"" $FILE
  ((LINE=LINE+1))
  sed -i "${LINE}d" $FILE
  sed -i "${LINE}i\#include \"$DCA_KERNEL_DIR/util/printing.h\"" $FILE
done
