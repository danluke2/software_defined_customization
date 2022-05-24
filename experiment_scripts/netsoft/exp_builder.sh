#!/bin/bash

#arg 1 is the module to build (without extension)
#arg2 is the module ID to assign
#arg3 is the host id
#arg4 is hex encoded key
#arg5 is active_mode flag
#arg6 is priority value
#arg7 is applyNow flag


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
# ************** END STANDARD PARAMS  ****************


# ----------------------------------------------------------------
# Function for delete lines between start params and end params
#   Accepts 1 argument:
#     File name
# ----------------------------------------------------------------
delete_lines()
{
  START="$(grep -n "NCO VARIABLES GO HERE" $1 | head -n 1 | cut -d: -f1)"
  END="$(grep -n "END NCO VARIABLES" $1 | head -n 1 | cut -d: -f1)"

  # delete all lines between start and end
  ((START=START+1))
  ((END=END-1))
  if [ $START -lt $END ]
  then
    sed -i "${START},${END}d" $1
  fi
}


CURDIR="$( pwd )"

line=$3;

symvers_dir=$GIT_DIR/../device_modules/host_$4
mod_dir=$symvers_dir/modules

#copy module from core dir to the host module dir before changing it
cp $NETSOFT_MOD_DIR/$1.c  $mod_dir

# delete all lines between start and end
delete_lines $mod_dir/${1}.c

#open module and insert NCO assigned values
sed -i "${line}i\u16 module_id=${2};" $mod_dir/${1}.c
((line=line+1))
sed -i "${line}i\char hex_key[HEX_KEY_LENGTH]=\"$4\";" $mod_dir/${1}.c
((line=line+1))
sed -i "${line}i\u16 activate=${5};" $mod_dir/${1}.c
((line=line+1))
sed -i "${line}i\u16 priority=${6};" $mod_dir/${1}.c
((line=line+1))
sed -i "${line}i\u16 applyNow=${7};" $mod_dir/${1}.c



#make the module based on host_id symver location
cd $mod_dir

symbols="KBUILD_EXTRA_SYMBOLS=$symvers_dir/Module.symvers"
mod="MODULE_DIR=$mod_dir"
build="BUILD_MODULE=$1.o"

make $symbols $mod $build

cd $CURDIR
