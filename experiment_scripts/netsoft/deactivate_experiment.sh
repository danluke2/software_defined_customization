#!/bin/bash

#Purpose: attach module, deprecate it and ensure stay active on current socket but not on new sockets
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
CUST_LOCATION=/usr/lib/modules/5.13.0-35-generic/layer4_5/customizations

SERVER_IP=10.0.0.20
SERVER_PASSWD=vagrant
CLIENT_IP=10.0.0.40
CLIENT_PASSWD=vagrant

# ************** STANDARD PARAMS MUST GO HERE ****************

# Force root
if [[ "$(id -u)" != "0" ]];
then
	echo "This script must be run as root" 1>&2
	exit -1
fi




echo "Installing Layer 4.5 on server and client"

sshpass -p "$SERVER_PASSWD" ssh -p 22 -o StrictHostKeyChecking=no root@$SERVER_IP "rmmod layer4_5; systemctl stop dnsmasq.service; systemctl stop systemd-resolved.service; $DCA_KERNEL_DIR/bash/installer.sh; dnsmasq --no-daemon -c 0 >/dev/null 2>&1 &"

sleep 1

rmmod layer4_5

$DCA_KERNEL_DIR/bash/installer.sh;

sleep 2


OUTPUT=$EXP_SCRIPT_DIR/logs/rotating_cust.txt
touch $OUTPUT

echo "*************** starting tap test ***************"


for ((i=1;i<=$1;i++))
do
  echo "DNS test $i"
  total=0;
  for ((j=1;j<=$2;j++))
  do
    query="www.test_tap$i$j.com"
    before=$(date '+%s%6N');
    dig @$SERVER_IP -p 53 $query > /dev/null;
    after=$(date '+%s%6N');
    total=$((total+(after-before)));
    sleep $3;
  done
  echo "$((total))" >> $OUTPUT;
done

echo "*************** finished tap test ***************"



# start NCO process with command line params
echo "*************** Starting NCO on Client  ***************"
gnome-terminal -- bash -c  "echo '*************** Starting NCO  ***************'; python3 $NCO_DIR/NCO.py --query_interval 5 --linear --print --ip $CLIENT_IP"

sleep 2

# start client DCA process, which will have host_id = 1
echo "*************** Starting DCA on Client  ***************"
gnome-terminal -- bash -c  "echo '*************** Client DCA  ***************'; python3 $DCA_USER_DIR/DCA.py --print --logfile $DCA_USER_DIR/client_dca_messages.log --ip $CLIENT_IP"

sleep 2


# start DCA process on server, which will have host_id = 2
echo "*************** Starting DCA on Server  ***************"
sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill dnsmasq; python3 $DCA_USER_DIR/DCA.py --ip $CLIENT_IP --logfile $DCA_USER_DIR/server_dca_messages.log >/dev/null 2>&1 &"

sleep 2


# Insert server module in DB for deployment to host_id = 2
echo "*************** Deploy Server Module  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "demo_dns_server_app_tag" --host 2

sleep 15


# Insert client module in DB for deployment to host_id = 1
echo "*************** Deploy Client Module  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "demo_dns_client_app_tag" --host 1

sleep 15


cd $NETSOFT_SCRIPT_DIR

echo "*************** Restarting DNSMASQ on Server  ***************"
sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "dnsmasq --no-daemon -c 0 >/dev/null 2>&1 &"

sleep 2

# start middlebox collection process
echo "*************** Starting Middlebox DCA on Client  ***************"
gnome-terminal -- bash -c  "echo '*************** Starting TCPDUMP  ***************'; tcpdump port 53 -i any -w $GIT_DIR/deprecate.pcap"

sleep 5


echo "*************** starting cust both ***************"

for ((i=1;i<=$1;i++))
do
  echo "DNS test $i"
  total=0;
  for ((j=1;j<=$2;j++))
  do
    query="www.test_both$i$j.com"
    before=$(date '+%s%6N');
    dig @$SERVER_IP -p 53 $query > /dev/null;
    after=$(date '+%s%6N');
    total=$((total+(after-before)));
    sleep $3;
  done
  echo "$((total))" >> $OUTPUT;
done

echo "*************** finished cust both ***************"


# deprecate server module in DB host_id = 2
echo "*************** Deprecate Server Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "demo_dns_server_app_tag" --host 2 --deprecate

sleep 15

# dnsmasq still has inactive module attached, so these should work fine
echo "*************** starting inactive test ***************"

for ((i=1;i<=$1;i++))
do
  echo "DNS test $i"
  total=0;
  for ((j=1;j<=$2;j++))
  do
    query="www.test_inactive$i$j.com"
    before=$(date '+%s%6N');
    dig @$SERVER_IP -p 53 $query > /dev/null;
    after=$(date '+%s%6N');
    total=$((total+(after-before)));
    sleep $3;
  done
  echo "$((total))" >> $OUTPUT;
done

echo "*************** finished inactive test  ***************"


# deprecate server module in DB host_id = 2
echo "*************** Deprecate Client Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "demo_dns_client_app_tag" --host 1 --deprecate

sleep 15


# now we restart dnsmasq, which should not get the module attached since inactive
echo "*************** Restarting DNSMASQ on Server  ***************"
sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill dnsmasq; dnsmasq --no-daemon -c 0 >/dev/null 2>&1 &"

sleep 2


echo "*************** starting no module test ***************"

for ((i=1;i<=$1;i++))
do
  echo "DNS test $i"
  total=0;
  for ((j=1;j<=$2;j++))
  do
    query="www.test_no_mod$i$j.com"
    before=$(date '+%s%6N');
    dig @$SERVER_IP -p 53 $query > /dev/null;
    after=$(date '+%s%6N');
    total=$((total+(after-before)));
    sleep $3;
  done
  echo "$((total))" >> $OUTPUT;
done

echo "*************** finished no module test  ***************"



# Revoke server module in DB host_id = 2
echo "*************** Revoke Server Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "demo_dns_server_app_tag" --host 2

sleep 2

# REvoke client module in DB host_id = 1
echo "*************** Revoke Client Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "demo_dns_client_app_tag" --host 1

sleep 15

echo "cleaning up"


rmmod layer4_5
pkill tcpdump

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill dnsmasq; systemctl start systemd-resolved.service; rmmod layer4_5"
