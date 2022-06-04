#!/bin/bash

#Purpose: #Purpose: cleans up files from experiments
#$1 is section to run

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

SERVER_IP=10.0.0.20
SERVER_PASSWD=vagrant
CLIENT_IP=10.0.0.40
CLIENT_PASSWD=vagrant

# ************** END STANDARD PARAMS ****************

WIRESHARK_DIR=/usr/lib/x86_64-linux-gnu/wireshark/plugins

# Bulk
if [ "$1" = "BULK" ]; then
  cd $EXP_SCRIPT_DIR/buffered
  rm buffer_bulk_overhead.png
  cd $EXP_SCRIPT_DIR/logs
  rm buffer_bulk_*.txt
  cd $LAYER_MOD_DIR/buffering
  make clean
fi

# Bulk TLS
if [ "$1" = "TLS" ]; then
  cd $EXP_SCRIPT_DIR/buffered
  rm buffer_tls_bulk_overhead.png
  cd $EXP_SCRIPT_DIR/logs
  rm buffer_tls_bulk_*.txt
  cd $LAYER_MOD_DIR/buffering
  make clean
fi

# Batch
if [ "$1" = "BATCH" ]; then
  cd $EXP_SCRIPT_DIR/buffered
  rm buffer_batch_overhead*.png
  cd $EXP_SCRIPT_DIR/logs
  rm buffer_batch_*.txt
  cd $LAYER_MOD_DIR/buffering
  make clean
fi
