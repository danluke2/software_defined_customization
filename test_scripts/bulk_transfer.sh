#!/bin/bash

#Purpose: perform $1 downloads of test file (number of trials to perform)

# need higher level script to setup each phase: base, tap, cust


# client connect to server over ssh, launch web server, then on client run experiment, save data to file

# create file to store download times and md5 sum
OUTPUT=logs/bulk_base.txt
touch $OUTPUT

sshpass -p "default" ssh -p 22 root@10.0.0.20 "rmmod layer4_5; pkill python; cd Desktop; nohup python3 ../software_defined_customization/test_scripts/client_server/python_simple_server.py; exit"

echo starting baseline downloads

for ((i=1;i<=$1;i++))
do
  echo "Download $i";
  before=$(date '+%s%3N');
  curl http://10.0.0.20:8080/overhead.iso -o overhead.iso;
  after=$(date '+%s%3N');
  echo "****** Time = $((after - before)) " >> $OUTPUT;
  sum=($(md5sum overhead.iso));
  echo "$sum" >> $OUTPUT;
  rm overhead.iso;
  sleep 2;
done

echo finished baseline test


# client connect to server over ssh, install L4.5, launch server, client install L4.5, run experiment, save data to file

# create file to store download times and md5 sum
OUTPUT=logs/bulk_tap.txt
touch $OUTPUT

sshpass -p "default" ssh -p 22 root@10.0.0.20 "pkill python; ./software_defined_customization/DCA_kernel/bash/installer.sh; cd Desktop; python3 ../software_defined_customization/test_scripts/client_server/python_simple_server.py; exit"

./software_defined_customization/DCA_kernel/bash/installer.sh;

echo starting tap downloads

for ((i=1;i<=$1;i++))
do
  echo "Download $i";
  before=$(date '+%s%3N');
  curl http://10.0.0.20:8080/overhead.iso -o overhead.iso;
  after=$(date '+%s%3N');
  echo "****** Time = $((after - before)) " >> $OUTPUT;
  sum=($(md5sum overhead.iso));
  echo "$sum" >> $OUTPUT;
  rm overhead.iso;
  sleep 2;
done

echo finished tap test


# client connect to server over ssh, install module, launch server, client install module, run experiment, save data to file

OUTPUT=logs/bulk_cust.txt
touch $OUTPUT

sshpass -p "default" ssh -p 22 root@10.0.0.20 "pkill python; cd software_defined_customization/test_modules; make BUILD_MODULE=overhead_test_bulk_file_server.o; insmod overhead_test_bulk_file_server;  cd Desktop; python3 ../software_defined_customization/test_scripts/client_server/python_simple_server.py; exit"

cd software_defined_customization/test_modules;
make BUILD_MODULE=overhead_test_bulk_file_client.o;
insmod overhead_test_bulk_file_client;

echo starting cust downloads

for ((i=1;i<=$1;i++))
do
  echo "Download $i";
  before=$(date '+%s%3N');
  curl http://10.0.0.20:8080/overhead.iso -o overhead.iso;
  after=$(date '+%s%3N');
  echo "****** Time = $((after - before)) " >> $OUTPUT;
  sum=($(md5sum overhead.iso));
  echo "$sum" >> $OUTPUT;
  rm overhead.iso;
  sleep 2;
done

echo finished cust test


# import data from file into this program to plot data





echo starting downloads

for ((i=1;i<=$1;i++))
do
  echo "Download $i";
  before=$(date '+%s%3N');
  curl http://10.0.0.20:8080/overhead.iso -o overhead.iso;
  after=$(date '+%s%3N');
  echo "****** Time = $((after - before)), " >> logs/bulk.txt;
  sum=($(md5sum overhead.iso));
  echo "START MD5 diff";
  diff  <(echo "$sum" ) <(echo "d14cb9b6f48feda0563cda7b5335e4c0");
  echo "END MD5 diff";
  rm overhead.iso;
  sleep 5;
done
