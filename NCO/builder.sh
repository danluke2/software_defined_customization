#!/bin/bash


#arg 1 is the module to build (without extension)
#arg2 is the module ID to assign
#arg3 is the line number to start inserting at
#arg4 is the host id
#arg5 is hex encoded key
#arg 6 is config info ...


CURDIR="$( pwd )"

line=$3;

core_mod_dir=core_modules
symvers_dir=$CURDIR/device_modules/host_$4
mod_dir=$symvers_dir/modules

#copy module from core dir to the host module dir before changing it
cp $core_mod_dir/$1.c  $mod_dir

#open module and replace insert u16 module_id = XX; with PCC assigned value
sed -i "${line}i\u16 module_id=${2};" $mod_dir/${1}.c
((line=line+1))
sed -i "${line}i\char hex_key[HEX_KEY_LENGTH]=\"$5\";" $mod_dir/${1}.c


#make the module based on host_id symver location
cd $mod_dir

symbols="KBUILD_EXTRA_SYMBOLS=$symvers_dir/Module.symvers"
mod="MODULE_DIR=$mod_dir"
build="BUILD_MODULE=$1.o"

make $symbols $mod $build

cd $CURDIR
