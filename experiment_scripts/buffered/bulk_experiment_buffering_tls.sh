#!/bin/bash

#Purpose: perform downloads of test file for each config
# $1 = number of downloads (number of trials to perform)

# ************** STANDARD PARAMS MUST GO HERE ****************
GIT_DIR=/home/vagrant/software_defined_customization
NCO_DIR=/home/vagrant/software_defined_customization/NCO
EXP_SCRIPT_DIR=/home/vagrant/software_defined_customization/experiment_scripts
BUFFER_SCRIPT_DIR=/home/vagrant/software_defined_customization/experiment_scripts/buffered
LAYER_MOD_DIR=/home/vagrant/software_defined_customization/layer4_5_modules
BUFFER_MOD_DIR=/home/vagrant/software_defined_customization/layer4_5_modules/buffering
SIMPLE_SERVER_DIR=/home/vagrant/software_defined_customization/experiment_scripts/client_server
DCA_KERNEL_DIR=/home/vagrant/software_defined_customization/DCA_kernel
DCA_USER_DIR=/home/vagrant/software_defined_customization/DCA_user
CUST_LOCATION=/usr/lib/modules/5.13.0-35-generic/layer4_5/customizations
SERVER_IP=10.0.0.20
SERVER_PASSWD=vagrant
CLIENT_IP=10.0.0.40
CLIENT_PASSWD=vagrant
# ************** END STANDARD PARAMS ****************

# Force root
if [[ "$(id -u)" != "0" ]]; then
  echo "This script must be run as root" 1>&2
  exit -1
fi

# ----------------------------------------------------------------
# Function for bulk test
#   Accepts 2 argument:
#     number of trials
#     output file
# ----------------------------------------------------------------
conduct_get() {
  for ((i = 1; i <= $1; i++)); do
    echo "Download $i"
    before=$(date '+%s%3N')
    curl -k https://$SERVER_IP/overhead.iso -o overhead.iso
    after=$(date '+%s%3N')
    sum=($(md5sum overhead.iso))
    echo "$sum" >>$2
    echo "$((after - before)) " >>$2
    rm overhead.iso
    sleep 2
  done
}

#check if overhead file is present before starting
FILE=$NCO_DIR/overhead.iso
if [ -f "$FILE" ]; then
  echo "$FILE check passed"
else
  echo "Could not find required overhead file" 1>&2
  exit -1
fi

echo "Calculating file checksum"
MD5=($(md5sum $NCO_DIR/overhead.iso))

# client connect to server over ssh, launch web server, then on client run experiment, save data to file

# create file to store download times and md5 sum
OUTPUT=$EXP_SCRIPT_DIR/logs/buffer_tls_bulk_base.txt
touch $OUTPUT
# store md5 sum at start of file for comparison
echo $MD5 >>$OUTPUT

sshpass -p "$SERVER_PASSWD" ssh -p 22 -o StrictHostKeyChecking=no root@$SERVER_IP "rmmod layer4_5; pkill python; cd $GIT_DIR/../Desktop; cp $NCO_DIR/overhead.iso .; python3 $SIMPLE_SERVER_DIR/python_https_server.py >/dev/null 2>&1 &"

sleep 1

rmmod layer4_5

sleep 1

echo "*************** starting baseline downloads ***************"

# download file to VM desktop to avoid using shared disk space
cd $GIT_DIR/../Desktop

conduct_get $1 $OUTPUT

echo "*************** finished baseline test ***************"

# client connect to server over ssh, install L4.5, launch server, client install L4.5, run experiment, save data to file

echo "Installing Layer 4.5 on server and client"

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "$DCA_KERNEL_DIR/bash/installer.sh; cd $GIT_DIR/../Desktop; python3 $SIMPLE_SERVER_DIR/python_https_server.py >/dev/null 2>&1 &"

sleep 1

$DCA_KERNEL_DIR/bash/installer.sh

sleep 2

OUTPUT=$EXP_SCRIPT_DIR/logs/buffer_tls_bulk_tap.txt
touch $OUTPUT
echo $MD5 >>$OUTPUT

echo "*************** starting tap downloads ***************"

# download file to VM desktop to avoid using shared disk space
cd $GIT_DIR/../Desktop

conduct_get $1 $OUTPUT

echo "*************** finished tap test ***************"

# client connect to server over ssh, install module, launch server, client install module, run experiment, save data to file

OUTPUT=$EXP_SCRIPT_DIR/logs/buffer_tls_bulk_cust.txt
touch $OUTPUT

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill python; cd $BUFFER_MOD_DIR; make BUILD_MODULE=overhead_test_bulk_file_tls_server.o; insmod overhead_test_bulk_file_tls_server.ko; cd $GIT_DIR/../Desktop; python3 $SIMPLE_SERVER_DIR/python_https_server.py >/dev/null 2>&1 &"

sleep 2
echo $MD5 >>$OUTPUT

cd $BUFFER_MOD_DIR
make BUILD_MODULE=overhead_test_bulk_file_tls_client.o
insmod overhead_test_bulk_file_tls_client.ko

sleep 2

echo "*************** starting cust downloads ***************"

# download file to VM desktop to avoid using shared disk space
cd $GIT_DIR/../Desktop

conduct_get $1 $OUTPUT

echo "*************** finished cust test ***************"

echo cleaning up

rmmod overhead_test_bulk_file_tls_client
rmmod layer4_5

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill python; rmmod overhead_test_bulk_file_tls_server; rmmod layer4_5; cd $GIT_DIR/../Desktop; rm overhead.iso; exit"

cd $BUFFER_SCRIPT_DIR

echo "generating plot"

python3 buffer_tls_bulk_plot.py
