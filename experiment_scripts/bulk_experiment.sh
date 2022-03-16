#!/bin/bash

#Purpose: perform $1 downloads of test file for each config (number of trials to perform)





# ************** STANDARD PARAMS MUST GO HERE ****************
GIT_DIR=/home/vagrant/software_defined_customization
NCO_DIR=$GIT_DIR/NCO
EXP_SCRIPT_DIR=$GIT_DIR/experiment_scripts
EXP_MOD_DIR=$GIT_DIR/experiment_modules
SIMPLE_SERVER_DIR=$EXP_SCRIPT_DIR/client_server
DCA_KERNEL_DIR=$GIT_DIR/DCA_kernel
DCA_USER_DIR=$GIT_DIR/DCA_user

SERVER_IP=10.0.0.20
SERVER_PASSWD="vagrant"
CLIENT_IP=10.0.0.40
CLIENT_PASSWD="vagrant"




# ************** STANDARD PARAMS MUST GO HERE ****************

# Force root
if [[ "$(id -u)" != "0" ]];
then
	echo "This script must be run as root" 1>&2
	exit -1
fi



MD5="d14cb9b6f48feda0563cda7b5335e4c0"

# client connect to server over ssh, launch web server, then on client run experiment, save data to file

# create file to store download times and md5 sum
OUTPUT=$EXP_SCRIPT_DIR/logs/bulk_base.txt
touch $OUTPUT

sshpass -p "$SERVER_PASSWD" ssh -p 22 -o StrictHostKeyChecking=no root@$SERVER_IP "rmmod layer4_5; pkill python; cd $NCO_DIR; python3 $SIMPLE_SERVER_DIR/python_simple_server.py >/dev/null 2>&1 &"

sleep 2

rmmod layer4_5

# store md5 sum at start of file for comparison
echo $MD5 >> $OUTPUT

echo "*************** starting baseline downloads ***************"

for ((i=1;i<=$1;i++))
do
  echo "Download $i";
  before=$(date '+%s%3N');
  curl http://$SERVER_IP:8080/overhead.iso -o overhead.iso;
  after=$(date '+%s%3N');
  sum=($(md5sum overhead.iso));
  echo "$sum" >> $OUTPUT;
  echo "$((after - before)) " >> $OUTPUT;
  rm overhead.iso;
  sleep 2;
done

echo "*************** finished baseline test ***************"


# client connect to server over ssh, install L4.5, launch server, client install L4.5, run experiment, save data to file

# create file to store download times and md5 sum
OUTPUT=$EXP_SCRIPT_DIR/logs/bulk_tap.txt
touch $OUTPUT

echo Installing Layer 4.5 on server and client

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill python; $DCA_KERNEL_DIR/bash/installer.sh $DCA_KERNEL_DIR; cd $NCO_DIR; python3 $SIMPLE_SERVER_DIR/python_simple_server.py >/dev/null 2>&1 &"

sleep 2
echo $MD5 >> $OUTPUT

$DCA_KERNEL_DIR/bash/installer.sh $DCA_KERNEL_DIR;

sleep 2

echo "*************** starting tap downloads ***************"

for ((i=1;i<=$1;i++))
do
  echo "Download $i";
  before=$(date '+%s%3N');
  curl http://$SERVER_IP:8080/overhead.iso -o overhead.iso;
  after=$(date '+%s%3N');
  sum=($(md5sum overhead.iso));
  echo "$sum" >> $OUTPUT;
  echo "$((after - before)) " >> $OUTPUT;
  rm overhead.iso;
  sleep 2;
done

echo "*************** finished tap test ***************"


# client connect to server over ssh, install module, launch server, client install module, run experiment, save data to file

OUTPUT=$EXP_SCRIPT_DIR/logs/bulk_cust.txt
touch $OUTPUT

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill python; cd $EXP_MOD_DIR; make BUILD_MODULE=overhead_test_bulk_file_server.o; insmod overhead_test_bulk_file_server.ko;  cd $NCO_DIR; python3 $SIMPLE_SERVER_DIR/python_simple_server.py >/dev/null 2>&1 &"


sleep 2
echo $MD5 >> $OUTPUT

cd $EXP_MOD_DIR;
make BUILD_MODULE=overhead_test_bulk_file_client.o;
insmod overhead_test_bulk_file_client.ko;
cd $EXP_SCRIPT_DIR

sleep 2
echo "*************** starting cust downloads ***************"

for ((i=1;i<=$1;i++))
do
  echo "Download $i";
  before=$(date '+%s%3N');
  curl http://$SERVER_IP:8080/overhead.iso -o overhead.iso;
  after=$(date '+%s%3N');
  sum=($(md5sum overhead.iso));
  echo "$sum" >> $OUTPUT;
  echo "$((after - before)) " >> $OUTPUT;
  rm overhead.iso;
  sleep 2;
done

echo "*************** finished cust test ***************"

echo cleaning up

rmmod overhead_test_bulk_file_client
rmmod layer4_5

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill python; rmmod overhead_test_bulk_file_server; rmmod layer4_5; exit"


echo generating plot

python3 bulk_plot.py
