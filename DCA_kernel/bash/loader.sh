#!/bin/bash


# check if layer4_5 is running before inserting other modules
# must be run with root permissions to load modules

MODULE="layer4_5"

# ************** STANDARD PARAMS MUST GO HERE ****************
INSTALLER_MAKEFILE_DIR=/home/vagrant/software_defined_customization/DCA_kernel
INSTALL_LOCATION=/usr/lib/modules/5.13.0-37-generic/layer4_5
CUST_LOCATION=/usr/lib/modules/5.13.0-37-generic/layer4_5/customizations
DCA_LOCATION=/usr/lib/modules/5.13.0-37-generic/layer4_5/DCA
DCA_USER_DIR=/home/vagrant/software_defined_customization/DCA_user



# ************** STANDARD PARAMS MUST GO HERE ****************

# use nullglob in case there are no matching files
shopt -s nullglob

cd $CUST_LOCATION
# create an array with all the filer/dir inside ~/myDir
arr=($CUST_LOCATION/*.ko)


## check is layer 4.5 loaded but only wait 120 seconds ##
for i in {1..12}
do
  if lsmod | grep "$MODULE" &> /dev/null ; then
    #echo "$MODULE is loaded!"
    # iterate through customization module array using a counter
    for ((i=0; i<${#arr[@]}; i++)); do
        #insert cust module
        insmod "${arr[$i]}"
    done
    exit 0
  else
    #echo "$MODULE is not loaded!"
    sleep 10s
  fi
done
