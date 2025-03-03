#!/bin/bash

#Purpose: perform downloads of test file for each config
# $1 = number of downloads (number of trials to perform)

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
CUST_LOCATION=/usr/lib/modules/$distro/layer4_5/customizations
SERVER_IP=192.168.0.18
SERVER_PASSWD=vagrant
CLIENT_IP=192.168.0.12
CLIENT_PASSWD=vagrant
# ************** END STANDARD PARAMS  ****************

# Force root
if [[ "$(id -u)" != "0" ]]; then
  echo "This script must be run as root" 1>&2
  exit -1
fi

# ----------------------------------------------------------------
# Function for bulk test
#   Accepts 1 argument:
#     number of trials
# ----------------------------------------------------------------
conduct_bulk() {
  for ((i = 1; i <= $1; i++)); do
    echo "Download $i"
    before=$(date '+%s%3N')
    curl http://$SERVER_IP:8080/overhead.iso -o overhead.iso
    after=$(date '+%s%3N')
    sum=($(md5sum overhead.iso))
    echo "$sum" >>$OUTPUT
    echo "$((after - before)) " >>$OUTPUT
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

sshpass -p "$SERVER_PASSWD" ssh -p 22 -o StrictHostKeyChecking=no root@$SERVER_IP "rmmod layer4_5; pkill python; cd $GIT_DIR/../Desktop; cp $NCO_DIR/overhead.iso .; python3 $SIMPLE_SERVER_DIR/python_simple_server.py >/dev/null 2>&1 &"

sleep 2

rmmod layer4_5

OUTPUT=$EXP_SCRIPT_DIR/logs/bulk_primer.txt
touch $OUTPUT
echo $MD5 >>$OUTPUT

echo "*************** starting baseline priming ***************"

conduct_bulk 1

echo "*************** finished baseline priming ***************"

# create file to store download times and md5 sum
OUTPUT=$EXP_SCRIPT_DIR/logs/bulk_base.txt
touch $OUTPUT
# store md5 sum at start of file for comparison
echo $MD5 >>$OUTPUT

echo "*************** starting baseline downloads ***************"

# download file to VM desktop to avoid using shared disk space
cd $GIT_DIR/../Desktop

conduct_bulk $1

echo "*************** finished baseline test ***************"

# client connect to server over ssh, install L4.5, launch server, client install L4.5, run experiment, save data to file

# create file to store download times and md5 sum
OUTPUT=$EXP_SCRIPT_DIR/logs/bulk_tap.txt
touch $OUTPUT

echo "Installing Layer 4.5 on server and client"

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill python; $DCA_KERNEL_DIR/bash/installer.sh; cd $GIT_DIR/../Desktop; python3 $SIMPLE_SERVER_DIR/python_simple_server.py >/dev/null 2>&1 &"

sleep 2
echo $MD5 >>$OUTPUT

$DCA_KERNEL_DIR/bash/installer.sh

sleep 2

echo "*************** starting tap downloads ***************"

# download file to VM desktop to avoid using shared disk space
cd $GIT_DIR/../Desktop

conduct_bulk $1

echo "*************** finished tap test ***************"

# client connect to server over ssh, install module, launch server, client install module, run experiment, save data to file

OUTPUT=$EXP_SCRIPT_DIR/logs/bulk_cust.txt
touch $OUTPUT

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill python; cd $NETSOFT_MOD_DIR; make module=overhead_test_bulk_file_server.o; insmod overhead_test_bulk_file_server.ko; cd $GIT_DIR/../Desktop; python3 $SIMPLE_SERVER_DIR/python_simple_server.py >/dev/null 2>&1 &"

sleep 2
echo $MD5 >>$OUTPUT

cd $NETSOFT_MOD_DIR
make module=overhead_test_bulk_file_client.o
insmod overhead_test_bulk_file_client.ko
cd $EXP_SCRIPT_DIR

sleep 2
echo "*************** starting cust downloads ***************"

# download file to VM desktop to avoid using shared disk space
cd $GIT_DIR/../Desktop

conduct_bulk $1

echo "*************** finished cust test ***************"

echo cleaning up

rmmod overhead_test_bulk_file_client
rmmod layer4_5

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill python; rmmod overhead_test_bulk_file_server; rmmod layer4_5; cd $GIT_DIR/../Desktop; rm overhead.iso; exit"

cd $NETSOFT_SCRIPT_DIR

echo generating plot

python3 bulk_plot.py
