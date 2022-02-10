#!/bin/bash


# check if layer4_5 is running before inserting other modules
# must be run with root permissions to load modules

MODULE="layer4_5"

# location where customization modules must exist to be loaded at runtime
INSTALL_LOCATION=/usr/lib/modules/$(uname -r)/layer4_5/customizations


# use nullglob in case there are no matching files
shopt -s nullglob

cd $INSTALL_LOCATION
# create an array with all the filer/dir inside ~/myDir
arr=($INSTALL_LOCATION/*.ko)


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
