#!/bin/bash

#Purpose: perform dns requests to test overhead of tagging for each config
# $1 = number of trials
# $2 = number of DNS requests per batch
# $3 = sleep time between each DNS request

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

# Force root
if [[ "$(id -u)" != "0" ]]; then
  echo "This script must be run as root" 1>&2
  exit -1
fi

# client connect to server over ssh, stop dns services, start dnsmasq, then on client run experiment, save data to file

sshpass -p "$SERVER_PASSWD" ssh -p 22 -o StrictHostKeyChecking=no root@$SERVER_IP "rmmod layer4_5; systemctl stop dnsmasq.service; systemctl stop systemd-resolved.service; dnsmasq --no-daemon -c 0 >/dev/null 2>&1 &"

sleep 2

rmmod layer4_5

# comment out if already ran baseline tests in non-buffering branch
OUTPUT=$EXP_SCRIPT_DIR/logs/batch_base.txt
touch $OUTPUT

echo "*************** starting base batch ***************"

for ((i = 1; i <= $1; i++)); do
  echo "DNS test $i"
  total=0
  for ((j = 1; j <= $2; j++)); do
    query="www.test_base_$i$j.com"
    before=$(date '+%s%6N')
    dig @$SERVER_IP -p 53 $query >/dev/null
    after=$(date '+%s%6N')
    total=$((total + (after - before)))
    sleep $3
  done
  echo "$((total))" >>$OUTPUT
done

echo "*************** finished base test ***************"

# client connect to server over ssh, kill dnsmasq, install L4.5, restart dnsmasq, client install L4.5, run experiment, save data to file

OUTPUT=$EXP_SCRIPT_DIR/logs/buffer_batch_tap.txt
touch $OUTPUT

echo "Installing Layer 4.5 on server and client"

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill dnsmasq; $DCA_KERNEL_DIR/bash/installer.sh; dnsmasq --no-daemon -c 0 >/dev/null 2>&1 &"

sleep 2

$DCA_KERNEL_DIR/bash/installer.sh

sleep 2

echo "*************** starting tap batch ***************"

for ((i = 1; i <= $1; i++)); do
  echo "DNS test $i"
  total=0
  for ((j = 1; j <= $2; j++)); do
    query="www.test_tap$i$j.com"
    before=$(date '+%s%6N')
    dig @$SERVER_IP -p 53 $query >/dev/null
    after=$(date '+%s%6N')
    total=$((total + (after - before)))
    sleep $3
  done
  echo "$((total))" >>$OUTPUT
done

echo "*************** finished tap test ***************"

# client connect to server over ssh, kill dnsmasq, install module, restart dnsmasq, client install module, run experiment, save data to file

OUTPUT=$EXP_SCRIPT_DIR/logs/buffer_batch_cust.txt
touch $OUTPUT

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill dnsmasq; cd $LAYER_MOD_DIR/buffering; make BUILD_MODULE=overhead_test_batch_dns_server_buffer.o; insmod overhead_test_batch_dns_server_buffer.ko; dnsmasq --no-daemon -c 0 >/dev/null 2>&1 &"

sleep 2

# Client module is same as netsoft version
cd $LAYER_MOD_DIR/buffering
make BUILD_MODULE=overhead_test_batch_dns_client.o
insmod overhead_test_batch_dns_client.ko
cd $EXP_SCRIPT_DIR/buffered

sleep 2
echo "*************** starting cust batch ***************"

for ((i = 1; i <= $1; i++)); do
  echo "DNS test $i"
  total=0
  for ((j = 1; j <= $2; j++)); do
    query="www.test_cust$i$j.com"
    before=$(date '+%s%6N')
    dig @$SERVER_IP -p 53 $query >/dev/null
    after=$(date '+%s%6N')
    total=$((total + (after - before)))
    sleep $3
  done
  echo "$((total))" >>$OUTPUT
done

echo "*************** finished cust test ***************"

sleep 2

echo cleaning up

rmmod overhead_test_batch_dns_client
rmmod layer4_5

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill dnsmasq; systemctl start systemd-resolved.service; rmmod overhead_test_batch_dns_server_buffer; rmmod layer4_5; exit"

echo generating plot

python3 buffer_batch_plot_simple.py
