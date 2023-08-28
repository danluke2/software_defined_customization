#!/bin/bash

#Purpose:  Sets the config values across files in this repo
# Default values will correspond to vagrant settings
# find start and end param lines, delete all lines between them, and then insert new params

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
TEST_SCRIPT_DIR=$EXP_SCRIPT_DIR/feature_test
SIMPLE_SERVER_DIR=$EXP_SCRIPT_DIR/client_server

LAYER_MOD_DIR=$GIT_DIR/layer4_5_modules
NCO_MOD_DIR=$LAYER_MOD_DIR/nco_modules
NETSOFT_MOD_DIR=$LAYER_MOD_DIR/netsoft
GENI_MOD_DIR=$LAYER_MOD_DIR/geni

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

GENI_USERNAME=$1
GENI_PASSWORD=$2

# ******************** UPDATE SECTION  ************************

# ----------------------------------------------------------------
# Function for delete lines between start params and end params
#   Accepts 1 argument:
#     File name
# ----------------------------------------------------------------
delete_lines() {
  START_ORIG="$(grep -n "STANDARD PARAMS MUST GO HERE" $1 | head -n 1 | cut -d: -f1)"
  END="$(grep -n "END STANDARD PARAMS" $1 | head -n 1 | cut -d: -f1)"

  # delete all lines between start and end
  ((START = START_ORIG + 1))
  ((END = END - 1))
  if [ $START -le $END ]; then
    sed -i "${START},${END}d" $1
  fi
  # Return the original start line
  echo $START_ORIG
}

# -----------------------------------------------------------------------------
# Function that inserts select parameters into a given installation file
#   Accepts 2 arguments:
#       File name
#       Array of text lines to be inserted
# ----------------------------------------------------------------
insert_params() {
    local file="$1"   # The file name passed in
    shift
    local vars=("$@")  # The array of lines to be added

    line=$(delete_lines "$file")
    # local line="$(grep -n "STANDARD PARAMS MUST GO HERE" $file | head -n 1 | cut -d: -f1)"
    ((line = line + 1))   
    for v in "${vars[@]}"; do
        sed -i "${line}i$v" $file
        ((line = line + 1))   
    done
}

# *************** Installer Makefile update ***************

FILE=$INSTALLER_MAKEFILE_DIR/Makefile
PARAMS=("INSTALLER_MAKEFILE_DIR=$INSTALLER_MAKEFILE_DIR"
    "INSTALL_LOCATION=$INSTALL_LOCATION"
    "DISTRO_DIR=$DISTRO_DIR")

# insert standard params
insert_params "$FILE" "${PARAMS[@]}"
#LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
#((LINE = LINE + 1))
#sed -i "${LINE}i\INSTALLER_MAKEFILE_DIR=$INSTALLER_MAKEFILE_DIR" $FILE
#((LINE = LINE + 1))
#sed -i "${LINE}i\INSTALL_LOCATION=$INSTALL_LOCATION" $FILE
#((LINE = LINE + 1))
#sed -i "${LINE}i\DISTRO_DIR=$DISTRO_DIR" $FILE

# *************** Installer Makefile update ***************

echo "Check 1"
# *************** Other Makefile updates ***************

MAKEFILE_PATHS="$LAYER_MOD_DIR/sample_modules  $GENI_MOD_DIR $NETSOFT_MOD_DIR"

for x in $MAKEFILE_PATHS; do
  FILE=$x/Makefile
  PARAMS=("KBUILD_EXTRA_SYMBOLS=$KBUILD_EXTRA_SYMBOLS"
      "MODULE_DIR=$x"
      "DISTRO_DIR=$DISTRO_DIR")

  insert_params "$FILE" "${PARAMS[@]}"
done

# *************** Other Makefile updates ***************
echo "Check 2"
# *************** Installer Bash updates ***************

INSTALLER_FILES="installer.sh loader.sh"
for x in $INSTALLER_FILES; do
  FILE=$INSTALLER_MAKEFILE_DIR/bash/${x}
  PARAMS=("INSTALLER_MAKEFILE_DIR=$INSTALLER_MAKEFILE_DIR"
        "INSTALL_LOCATION=$INSTALL_LOCATION"
        "INCLUDE_DIR=$INCLUDE_DIR"
        "CUST_LOCATION=$CUST_LOCATION"
        "GIT_DIR=$GIT_DIR"
        "DCA_LOCATION=$DCA_LOCATION"
        "DCA_USER_DIR=$DCA_USER_DIR")

  insert_params "$FILE" "${PARAMS[@]}"
#  # insert standard params
#  LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\INSTALLER_MAKEFILE_DIR=$INSTALLER_MAKEFILE_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\INSTALL_LOCATION=$INSTALL_LOCATION" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\INCLUDE_DIR=$INCLUDE_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\CUST_LOCATION=$CUST_LOCATION" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\GIT_DIR=$GIT_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\DCA_LOCATION=$DCA_LOCATION" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\DCA_USER_DIR=$DCA_USER_DIR" $FILE
done

# *************** Installer Bash updates ***************
echo "Check 3"
# *************** DCA_user Bash updates ***************

INSTALLER_FILES="service.sh"
for x in $INSTALLER_FILES; do
  FILE=$DCA_USER_DIR/$x
  PARAMS=("DCA_LOCATION=$DCA_LOCATION")

  insert_params "$FILE" "${PARAMS[@]}"

#  # insert standard params
#  LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\DCA_LOCATION=$DCA_LOCATION" $FILE
done

# *************** DCA_user Bash updates ***************
echo "Check 4"
# *************** NCO Bash updates ***************

INSTALLER_FILES="service.sh"
for x in $INSTALLER_FILES; do
  FILE=$NCO_DIR/$x
  PARAMS=("NCO_DIR=$NCO_DIR")

  insert_params "$FILE" "${PARAMS[@]}"
#  # insert standard params
#  LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\NCO_DIR=$NCO_DIR" $FILE
done

# *************** NCO Bash Bash updates ***************
echo "Check 5"
# *************** Python simple server Bash update ***************

INSTALLER_FILES="service.sh"
for x in $INSTALLER_FILES; do
  FILE=$SIMPLE_SERVER_DIR/$x
  PARAMS=("SIMPLE_SERVER_DIR=$SIMPLE_SERVER_DIR")

  insert_params "$FILE" "${PARAMS[@]}"
#  # insert standard params
#  LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\SIMPLE_SERVER_DIR=$SIMPLE_SERVER_DIR" $FILE
done

# *************** Python simple server Bash updates ***************
echo "Check 6"
# *************** Python https server update ***************

INSTALLER_FILES="python_https_server.py"
for x in $INSTALLER_FILES; do
  FILE=$SIMPLE_SERVER_DIR/$x
  PARAMS=("HOST='$SERVER_IP'"
        "SIMPLE_SERVER_DIR='$SIMPLE_SERVER_DIR'")

  insert_params "$FILE" "${PARAMS[@]}"
#  # insert standard params
#  LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\HOST='$SERVER_IP'" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\SIMPLE_SERVER_DIR='$SIMPLE_SERVER_DIR'" $FILE
done

# *************** Python https server updates ***************
echo "Check 7"
# *************** Vagrant Bash updates ***************

FILE=$GIT_DIR/vagrant/setup.sh
PARAMS=("GIT_DIR=$GIT_DIR"
        "DCA_KERNEL_DIR=$DCA_KERNEL_DIR"
        "SIMPLE_SERVER_DIR=$SIMPLE_SERVER_DIR")

insert_params "$FILE" "${PARAMS[@]}"
## insert standard params
#LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
#((LINE = LINE + 1))
#sed -i "${LINE}i\GIT_DIR=$GIT_DIR" $FILE
#((LINE = LINE + 1))
#sed -i "${LINE}i\DCA_KERNEL_DIR=$DCA_KERNEL_DIR" $FILE
#((LINE = LINE + 1))
#sed -i "${LINE}i\SIMPLE_SERVER_DIR=$SIMPLE_SERVER_DIR" $FILE

# *************** Installer Bash updates ***************
echo "Check 8"
# *************** NetSoft Bash updates ***************

# list of files to modify
FILES="batch_experiment.sh bulk_experiment.sh challenge_response.sh cleanup.sh exp_builder.sh middle_demo.sh nco_dca_batch_experiment.sh nco_dca_experiment.sh"

for x in $FILES; do
  FILE=$NETSOFT_SCRIPT_DIR/${x}
  PARAMS=("GIT_DIR=$GIT_DIR"
        "NCO_DIR=$NCO_DIR"
        "EXP_SCRIPT_DIR=$EXP_SCRIPT_DIR"
        "NETSOFT_SCRIPT_DIR=$NETSOFT_SCRIPT_DIR"
        "GENI_SCRIPT_DIR=$GENI_SCRIPT_DIR"
        "LAYER_MOD_DIR=$LAYER_MOD_DIR"
        "NETSOFT_MOD_DIR=$NETSOFT_MOD_DIR"
        "GENI_MOD_DIR=$GENI_MOD_DIR"
        "SIMPLE_SERVER_DIR=$SIMPLE_SERVER_DIR"
        "DCA_KERNEL_DIR=$DCA_KERNEL_DIR"
        "DCA_USER_DIR=$DCA_USER_DIR"
        "CUST_LOCATION=$CUST_LOCATION"
        "SERVER_IP=$SERVER_IP"
        "SERVER_PASSWD=$SERVER_PASSWD"
        "CLIENT_IP=$CLIENT_IP"
        "CLIENT_PASSWD=$CLIENT_PASSWD")

  insert_params "$FILE" "${PARAMS[@]}"

#  # insert standard params
#  LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\GIT_DIR=$GIT_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\NCO_DIR=$NCO_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\EXP_SCRIPT_DIR=$EXP_SCRIPT_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\NETSOFT_SCRIPT_DIR=$NETSOFT_SCRIPT_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\GENI_SCRIPT_DIR=$GENI_SCRIPT_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\LAYER_MOD_DIR=$LAYER_MOD_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\NETSOFT_MOD_DIR=$NETSOFT_MOD_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\GENI_MOD_DIR=$GENI_MOD_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\SIMPLE_SERVER_DIR=$SIMPLE_SERVER_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\DCA_KERNEL_DIR=$DCA_KERNEL_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\DCA_USER_DIR=$DCA_USER_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\CUST_LOCATION=$CUST_LOCATION" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\SERVER_IP=$SERVER_IP" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\SERVER_PASSWD=$SERVER_PASSWD" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\CLIENT_IP=$CLIENT_IP" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\CLIENT_PASSWD=$CLIENT_PASSWD" $FILE

done

# *************** NetSoft Bash updates ***************
echo "Check 9"
# *************** Feature Test Bash updates ***************

# list of files to modify
FILES="activate_test.sh challenge_response_test.sh deprecate_test.sh priority_test.sh cleanup.sh"

for x in $FILES; do
  FILE=$TEST_SCRIPT_DIR/${x}
  insert_params "$FILE" "${PARAMS[@]}"

done
# *************** Feature Test Bash updates ***************
echo "Check 10"
# *************** GENI Bash updates ***************

# list of files to modify
FILES="middlebox_batch.sh middlebox_dns_single.sh middlebox_web_single.sh middlebox_dns_public.sh"

for x in $FILES; do
  FILE=$GENI_SCRIPT_DIR/${x}
  PARAMS=("GIT_DIR=$GIT_DIR"
        "NCO_DIR=$NCO_DIR"
        "EXP_SCRIPT_DIR=$EXP_SCRIPT_DIR"
        "NETSOFT_SCRIPT_DIR=$NETSOFT_SCRIPT_DIR"
        "GENI_SCRIPT_DIR=$GENI_SCRIPT_DIR"
        "LAYER_MOD_DIR=$LAYER_MOD_DIR"
        "NETSOFT_MOD_DIR=$NETSOFT_MOD_DIR"
        "GENI_MOD_DIR=$GENI_MOD_DIR"
        "SIMPLE_SERVER_DIR=$SIMPLE_SERVER_DIR"
        "DCA_KERNEL_DIR=$DCA_KERNEL_DIR"
        "DCA_USER_DIR=$DCA_USER_DIR"
        "USERNAME=$GENI_USERNAME"
        "PASSWORD=$GENI_PASSWORD")
  insert_params "$FILE" "${PARAMS[@]}"

#  # insert standard params
#  LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\GIT_DIR=$GIT_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\NCO_DIR=$NCO_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\EXP_SCRIPT_DIR=$EXP_SCRIPT_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\NETSOFT_SCRIPT_DIR=$NETSOFT_SCRIPT_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\GENI_SCRIPT_DIR=$GENI_SCRIPT_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\LAYER_MOD_DIR=$LAYER_MOD_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\NETSOFT_MOD_DIR=$NETSOFT_MOD_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\GENI_MOD_DIR=$GENI_MOD_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\SIMPLE_SERVER_DIR=$SIMPLE_SERVER_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\DCA_KERNEL_DIR=$DCA_KERNEL_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\DCA_USER_DIR=$DCA_USER_DIR" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\USERNAME=$GENI_USERNAME" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\PASSWORD=$GENI_PASSWORD" $FILE
done

# *************** GENI Bash updates ***************
echo "Check 11"
# *************** NCO Builder update ***************

FILE=$NCO_DIR/builder.sh
PARAMS=("NCO_DIR=$NCO_DIR"
        "NCO_MOD_DIR=$NCO_MOD_DIR")

insert_params "$FILE" "${PARAMS[@]}"

## insert standard params
#LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
#((LINE = LINE + 1))
#sed -i "${LINE}i\NCO_DIR=$NCO_DIR" $FILE
#((LINE = LINE + 1))
#sed -i "${LINE}i\NCO_MOD_DIR=$NCO_MOD_DIR" $FILE

# *************** NCO Builder update ***************
echo "Check 12"
# *************** NCO cfg update ***************

FILE=$NCO_DIR/cfg.py
PARAMS=("HOST='$SERVER_IP'"
        "nco_dir='$NCO_DIR/'"
        "nco_mod_dir='$NCO_MOD_DIR/'"
        "common_struct_dir='$DCA_KERNEL_DIR/'")
insert_params "$FILE" "${PARAMS[@]}"

## insert standard params
#LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
#((LINE = LINE + 1))
#sed -i "${LINE}i\HOST='$SERVER_IP'" $FILE
#((LINE = LINE + 1))
#sed -i "${LINE}i\nco_dir='$NCO_DIR/'" $FILE
#((LINE = LINE + 1))
#sed -i "${LINE}i\nco_mod_dir='$NCO_MOD_DIR/'" $FILE
#((LINE = LINE + 1))
#sed -i "${LINE}i\common_struct_dir='$DCA_KERNEL_DIR/'" $FILE

# *************** NCO cfg update ***************
echo "Check 13"
# *************** DCA cfg update ***************

FILE=$DCA_USER_DIR/cfg.py
PARAMS=("HOST='$SERVER_IP'"
        "dca_dir='$DCA_USER_DIR/'"
        "symver_location='$INSTALL_LOCATION/'"
        "system_release='$distro'")
insert_params "$FILE" "${PARAMS[@]}"

## insert standard params
#LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
#((LINE = LINE + 1))
#sed -i "${LINE}i\HOST='$SERVER_IP'" $FILE
#((LINE = LINE + 1))
#sed -i "${LINE}i\dca_dir='$DCA_USER_DIR/'" $FILE
#((LINE = LINE + 1))
#sed -i "${LINE}i\symver_location='$INSTALL_LOCATION/'" $FILE
#((LINE = LINE + 1))
#sed -i "${LINE}i\system_release='$distro'" $FILE

# *************** DCA cfg update ***************
echo "Check 14"
# *************** Module Include updates ***************
PARAMS=("#include <common_structs.h>"
        "#include <helpers.h>")

# sample modules
SAMPLE_MODULE_FILES="sample_python_client.c sample_python_server.c"
for x in $SAMPLE_MODULE_FILES; do
  FILE=$LAYER_MOD_DIR/sample_modules/${x}
  insert_params "$FILE" "${PARAMS[@]}"

#  # insert standard params
#  LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\#include <common_structs.h>" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\#include <helpers.h>" $FILE
done

# netsoft modules
NETSOFT_MODULE_FILES="overhead_test_batch_dns_client.c overhead_test_batch_dns_server.c overhead_test_bulk_file_client.c overhead_test_bulk_file_server.c"
for x in $NETSOFT_MODULE_FILES; do
  FILE=$NETSOFT_MOD_DIR/${x}
  insert_params "$FILE" "${PARAMS[@]}"

#  # insert standard params
#  LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\#include <common_structs.h>" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\#include <helpers.h>" $FILE
done

# geni modules
GENI_MODULE_FILES="bulk_client.c bulk_server.c compress_dns_client.c compress_dns_server.c end_dns_client.c end_dns_server.c front_dns_client.c front_dns_server.c middle_dns_client.c middle_dns_server.c"
for x in $GENI_MODULE_FILES; do
  FILE=$GENI_MOD_DIR/${x}
  insert_params "$FILE" "${PARAMS[@]}"

#  # insert standard params
#  LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\#include <common_structs.h>" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\#include <helpers.h>" $FILE
done

# extra modules
EXTRA_MODULE_FILES="sample_tls_client.c sample_tls_server.c"
for x in $EXTRA_MODULE_FILES; do
  FILE=$LAYER_MOD_DIR/extra/${x}
  insert_params "$FILE" "${PARAMS[@]}"

#  # insert standard params
#  LINE="$(grep -n "STANDARD PARAMS MUST GO HERE" $FILE | head -n 1 | cut -d: -f1)"
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\#include <common_structs.h>" $FILE
#  ((LINE = LINE + 1))
#  sed -i "${LINE}i\#include <helpers.h>" $FILE
done
echo "Last line!"
