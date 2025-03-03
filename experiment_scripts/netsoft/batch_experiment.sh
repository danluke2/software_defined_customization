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
# Function for dns test
#   Accepts 3 arguments:
#     number of trials
#     number of DNS requests
#     sleep interval between test
#     test name
# ----------------------------------------------------------------
conduct_dns() {
  for ((i = 1; i <= $1; i++)); do
    echo "DNS test $i"
    total=0
    for ((j = 1; j <= $2; j++)); do
      query="www.test_$3_$j.com"
      before=$(date '+%s%6N')
      dig @$SERVER_IP -p 53 $query >/dev/null
      after=$(date '+%s%6N')
      total=$((total + (after - before)))
      sleep $3
    done
    echo "$((total))" >>$OUTPUT
  done
}

# client connect to server over ssh, stop dns services, start dnsmasq, then on client run experiment, save data to file

sshpass -p "$SERVER_PASSWD" ssh -p 22 -o StrictHostKeyChecking=no root@$SERVER_IP "rmmod layer4_5; systemctl stop dnsmasq.service; systemctl stop systemd-resolved.service; sleep 5; dnsmasq --no-daemon -c 0 >/dev/null 2>&1 &"

sleep 2

rmmod layer4_5

OUTPUT=$EXP_SCRIPT_DIR/logs/batch_primer.txt
touch $OUTPUT

echo "*************** starting baseline priming ***************"

conduct_dns 1 $2 0 "prime"

echo "*************** finished baseline priming ***************"

# create file to store batch times
OUTPUT=$EXP_SCRIPT_DIR/logs/batch_base.txt
touch $OUTPUT

echo "*************** starting baseline batch ***************"

conduct_dns $1 $2 $3 "base"

echo "*************** finished baseline test ***************"

# client connect to server over ssh, kill dnsmasq, install L4.5, restart dnsmasq, client install L4.5, run experiment, save data to file

OUTPUT=$EXP_SCRIPT_DIR/logs/batch_tap.txt
touch $OUTPUT

echo Installing Layer 4.5 on server and client

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill dnsmasq; $DCA_KERNEL_DIR/bash/installer.sh; dnsmasq --no-daemon -c 0 >/dev/null 2>&1 &"

sleep 2

$DCA_KERNEL_DIR/bash/installer.sh

sleep 2

echo "*************** starting tap batch ***************"

conduct_dns $1 $2 $3 "tap"

echo "*************** finished tap test ***************"

# client connect to server over ssh, kill dnsmasq, install module, restart dnsmasq, client install module, run experiment, save data to file

OUTPUT=$EXP_SCRIPT_DIR/logs/batch_cust.txt
touch $OUTPUT

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill dnsmasq; cd $NETSOFT_MOD_DIR; make module=overhead_test_batch_dns_server.o; insmod overhead_test_batch_dns_server.ko; dnsmasq --no-daemon -c 0 >/dev/null 2>&1 &"

sleep 2

cd $NETSOFT_MOD_DIR
make module=overhead_test_batch_dns_client.o
insmod overhead_test_batch_dns_client.ko
cd $NETSOFT_SCRIPT_DIR

sleep 2

echo "*************** starting cust batch ***************"

conduct_dns $1 $2 $3 "cust"

echo "*************** finished cust test ***************"

sleep 2

echo cleaning up

rmmod overhead_test_batch_dns_client
rmmod layer4_5

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill dnsmasq; systemctl start systemd-resolved.service; rmmod overhead_test_batch_dns_server; rmmod layer4_5; exit"

echo generating plot

python3 batch_plot.py
