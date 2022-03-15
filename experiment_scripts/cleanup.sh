#!/bin/bash

#Purpose: #Purpose: cleans up files from experiments
#$1 is section to run

#directory holding the software_defined_customization git repo
GIT_DIR=/home/vagrant/software_defined_customization



#NCO/DCA
if [ "$1" = "NCO" ]; then
  cd $GIT_DIR/experiment_scripts
  rm nco_deploy.png
  rm cib.db
  cd logs
  rm n*
  cd $GIT_DIR/experiment_modules/device_modules
  rm -rf h*
fi



#Bulk
if [ "$1" = "BULK" ]; then
  cd $GIT_DIR/experiment_scripts
  rm bulk_overhead.png
  cd logs
  rm b*
  cd $GIT_DIR/experiment_modules
  make clean
fi


#Batch
if [ "$1" = "BATCH" ]; then
  cd $GIT_DIR/experiment_scripts
  rm batch_overhead.png
  cd logs
  rm b*
  cd $GIT_DIR/experiment_modules
  make clean
fi


#Challenge
if [ "$1" = "CHALLENGE" ]; then
  cd $GIT_DIR/NCO
  rm cib.db
  cd device_modules
  rm -rf h*
fi


#Middlebox
if [ "$1" = "MIDDLEBOX" ]; then
  cd $GIT_DIR/NCO
  rm cib.db
  cd device_modules
  rm -rf h*
  cd $GIT_DIR
  rm middle_demo.pcap
fi
