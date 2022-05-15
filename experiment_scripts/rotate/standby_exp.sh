#!/bin/bash

#Purpose: attach module in standby mode and ensure it attaches to socket but not customize it
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

sshpass -p "$SERVER_PASSWD" ssh -p 22 -o StrictHostKeyChecking=no root@$SERVER_IP "pkill python3; rmmod layer4_5; systemctl stop dnsmasq.service; systemctl stop systemd-resolved.service; $DCA_KERNEL_DIR/bash/installer.sh; dnsmasq --no-daemon -c 0 >/dev/null 2>&1 &"

sleep 1

rmmod layer4_5

$DCA_KERNEL_DIR/bash/installer.sh;

sleep 2

OUTPUT=$EXP_SCRIPT_DIR/logs/standby_exp.txt
touch $OUTPUT


# start NCO process with command line params
echo "*************** Starting NCO on Client  ***************"
gnome-terminal -- bash -c  "echo '*************** Starting NCO  ***************'; python3 $NCO_DIR/NCO.py --query_interval 5 --linear --print --ip $CLIENT_IP"


sleep 5 # give plenty of time to start up

# start client DCA process, which will have host_id = 1
echo "*************** Starting DCA on Client  ***************"
gnome-terminal -- bash -c  "echo '*************** Client DCA  ***************'; python3 $DCA_USER_DIR/DCA.py --print --logfile $DCA_USER_DIR/client_dca_messages.log --ip $CLIENT_IP"

sleep 5



# Insert client module in DB for deployment to host_id = 1, with standby mode set
echo "*************** Deploy Client Module  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "demo_dns_client_app_tag" --host 1 --standby 

sleep 10


# start DCA process on server, which will have host_id = 2
echo "*************** Starting DCA on Server  ***************"
sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "python3 $DCA_USER_DIR/DCA.py --ip $CLIENT_IP --logfile $DCA_USER_DIR/server_dca_messages.log  >/dev/null 2>&1 &"

sleep 5


# Insert server module in DB for deployment to host_id = 2, but now in standby mode with applyNow set
echo "*************** Deploy Server Module  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "demo_dns_server_app_tag" --host 2 --standby --applyNow

sleep 10


# start middlebox collection process
echo "*************** Starting Middlebox DCA on Client  ***************"
gnome-terminal -- bash -c  "echo '*************** Starting TCPDUMP  ***************'; tcpdump port 53 -i any -w $GIT_DIR/standby.pcap"

sleep 5


echo "*************** starting applyNow and standby test ***************"

for ((i=1;i<=$1;i++))
do
  echo "DNS test $i"
  total=0;
  for ((j=1;j<=$2;j++))
  do
    query="www.test_applyNow$i$j.com"
    before=$(date '+%s%6N');
    dig @$SERVER_IP -p 53 $query > /dev/null;
    after=$(date '+%s%6N');
    total=$((total+(after-before)));
    sleep $3;
  done
  echo "$((total))" >> $OUTPUT;
done

echo "*************** finished applyNow and standby test ***************"




# now we restart dnsmasq, which should still not get customized since in standby mode
echo "*************** Restarting DNSMASQ on Server  ***************"
sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill dnsmasq; dnsmasq --no-daemon -c 0 >/dev/null 2>&1 &"

sleep 2


echo "*************** starting standby only test ***************"

for ((i=1;i<=$1;i++))
do
  echo "DNS test $i"
  total=0;
  for ((j=1;j<=$2;j++))
  do
    query="www.test_standby$i$j.com"
    before=$(date '+%s%6N');
    dig @$SERVER_IP -p 53 $query > /dev/null;
    after=$(date '+%s%6N');
    total=$((total+(after-before)));
    sleep $3;
  done
  echo "$((total))" >> $OUTPUT;
done

echo "*************** finished standby only test  ***************"



# Now we enable the server and client modules and make sure they are applied 
echo "*************** Toggle Server Module On ***************"
python3 $NCO_DIR/toggle_module_helper.py --module "demo_dns_server_app_tag" --host 2 

sleep 2


echo "*************** Toggle Client Module On ***************"
python3 $NCO_DIR/toggle_module_helper.py --module "demo_dns_client_app_tag" --host 1 

sleep 15 # give plenty of time for module to reflect new standby status


echo "*************** starting cust enabled test ***************"

for ((i=1;i<=$1;i++))
do
  echo "DNS test $i"
  total=0;
  for ((j=1;j<=$2;j++))
  do
    query="www.test_cust$i$j.com"
    before=$(date '+%s%6N');
    dig @$SERVER_IP -p 53 $query > /dev/null;
    after=$(date '+%s%6N');
    total=$((total+(after-before)));
    sleep $3;
  done
  echo "$((total))" >> $OUTPUT;
done

echo "*************** finished cust enabled test ***************"

sleep 2


# Revoke server module in DB host_id = 2
echo "*************** Revoke Server Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "demo_dns_server_app_tag" --host 2

sleep 2

# Revoke client module in DB host_id = 1
echo "*************** Revoke Client Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "demo_dns_client_app_tag" --host 1

sleep 15

echo "cleaning up"


rmmod layer4_5
pkill tcpdump


sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill python3; pkill dnsmasq; systemctl start systemd-resolved.service; rmmod layer4_5"
