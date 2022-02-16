#!/bin/bash

#Purpose: perform $1 downloads of test file for each config (number of trials to perform)

#directory with overhead.iso
SERVER_DIR=/home/dan/Desktop
#directory holding the software_defined_customization git repo
GIT_DIR=/home/dan/software_defined_customization
SIMPLE_SERVER_DIR=$GIT_DIR/test_scripts/client_server


# client connect to server over ssh, launch web server, then on client run experiment, save data to file

# create file to store download times and md5 sum
OUTPUT=logs/bulk_base.txt
touch $OUTPUT

sshpass -p "default" ssh -p 22 root@10.0.0.20 "pkill python ; cd $SERVER_DIR ; python3 $SIMPLE_SERVER_DIR/python_simple_server.py >/dev/null 2>&1 &"

sleep 2
# store md5 sum at start of file for comparison
echo "d14cb9b6f48feda0563cda7b5335e4c0" >> $OUTPUT

echo "*************** starting baseline downloads ***************"

for ((i=1;i<=$1;i++))
do
  echo "Download $i";
  before=$(date '+%s%3N');
  curl http://10.0.0.20:8080/overhead.iso -o overhead.iso;
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
OUTPUT=logs/bulk_tap.txt
touch $OUTPUT

echo Installing Layer 4.5 on server and client

sshpass -p "default" ssh -p 22 root@10.0.0.20 "pkill python; $GIT_DIR/DCA_kernel/bash/installer.sh; cd $SERVER_DIR; python3 $SIMPLE_SERVER_DIR/python_simple_server.py >/dev/null 2>&1 &"

sleep 2
echo "d14cb9b6f48feda0563cda7b5335e4c0" >> $OUTPUT

/home/dan/software_defined_customization/DCA_kernel/bash/installer.sh;

sleep 2

echo "*************** starting tap downloads ***************"

for ((i=1;i<=$1;i++))
do
  echo "Download $i";
  before=$(date '+%s%3N');
  curl http://10.0.0.20:8080/overhead.iso -o overhead.iso;
  after=$(date '+%s%3N');
  sum=($(md5sum overhead.iso));
  echo "$sum" >> $OUTPUT;
  echo "$((after - before)) " >> $OUTPUT;
  rm overhead.iso;
  sleep 2;
done

echo "*************** finished tap test ***************"


# client connect to server over ssh, install module, launch server, client install module, run experiment, save data to file

OUTPUT=logs/bulk_cust.txt
touch $OUTPUT

sshpass -p "default" ssh -p 22 root@10.0.0.20 "pkill python; cd $GIT_DIR/test_modules; make BUILD_MODULE=overhead_test_bulk_file_server.o; insmod overhead_test_bulk_file_server.ko;  cd $SERVER_DIR; python3 $SIMPLE_SERVER_DIR/python_simple_server.py >/dev/null 2>&1 &"


sleep 2
echo "d14cb9b6f48feda0563cda7b5335e4c0" >> $OUTPUT

cd ../test_modules;
make BUILD_MODULE=overhead_test_bulk_file_client.o;
insmod overhead_test_bulk_file_client.ko;
cd ../test_scripts

sleep 2
echo "*************** starting cust downloads ***************"

for ((i=1;i<=$1;i++))
do
  echo "Download $i";
  before=$(date '+%s%3N');
  curl http://10.0.0.20:8080/overhead.iso -o overhead.iso;
  after=$(date '+%s%3N');
  sum=($(md5sum overhead.iso));
  echo "$sum" >> $OUTPUT;
  echo "$((after - before)) " >> $OUTPUT;
  rm overhead.iso;
  sleep 2;
done

echo "*************** finished cust test ***************"

echo cleaning up

sudo rmmod overhead_test_bulk_file_client
sudo rmmod layer4_5

sshpass -p "default" ssh -p 22 root@10.0.0.20 "pkill python; rmmod overhead_test_bulk_file_server; rmmod layer4_5; exit"


echo generating plot

python3 bulk_plot.py
