#!/bin/bash

#Purpose: perform the middlebox demo
# $1 = standard report interval (ex: 5)




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


SLEEP_INT=5

#install Layer 4.5 on each device
echo "*************** Install L4.5 on Client  ***************"

sshpass -p "$CLIENT_PASSWD" ssh -p 22 -o StrictHostKeyChecking=no root@$CLIENT_IP "rmmod layer4_5; $DCA_KERNEL_DIR/bash/installer.sh;"

sleep $SLEEP_INT


echo "*************** Install L4.5 on Server  ***************"

#ensure layer4.5 is not currently running and DB doesn't exist
rmmod layer4_5
rm $NCO_DIR/cib.db
pkill dnsmasq

$DCA_KERNEL_DIR/bash/installer.sh

sleep $SLEEP_INT


# start NCO process with command line params
echo "*************** Starting NCO  ***************"
gnome-terminal -- bash -c  "echo '*************** Starting NCO  ***************'; python3 $NCO_DIR/NCO.py --query_interval $1 --linear --print"

sleep $SLEEP_INT


# start DCA process, which will have host_id = 1
echo "*************** Starting DCA on Server  ***************"
gnome-terminal -- bash -c  "echo '*************** Server DCA  ***************'; python3 $DCA_USER_DIR/DCA.py --print --logfile $DCA_USER_DIR/server_dca_messages.log"

sleep $SLEEP_INT


# start DCA middlebox process
echo "*************** Starting Middlebox DCA on Server  ***************"
gnome-terminal -- bash -c  "echo '*************** Middlebox DCA  ***************';  python3 $DCA_USER_DIR/DCA_middlebox.py --print --logfile $DCA_USER_DIR/middlebox_dca_messages.log"

sleep $SLEEP_INT


# start DCA process on client, which will have host_id = 2
echo "*************** Starting DCA on Client  ***************"
# sshpass -p "$CLIENT_PASSWD" ssh -p 22 root@$CLIENT_IP "python3 $DCA_USER_DIR/DCA.py >/dev/null 2>&1 &"
sshpass -p "$CLIENT_PASSWD" ssh -p 22 root@$CLIENT_IP "python3 $DCA_USER_DIR/DCA.py --logfile $DCA_USER_DIR/client_dca_messages.log >/dev/null 2>&1 &"

sleep $SLEEP_INT


# Insert server module in DB for deployment to host_id = 1
echo "*************** Deploy Server Module and Inverse  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "demo_dns_server_app_tag" --host 1 --inverse "demo_dns_tag.lua" --type "Wireshark"

sleep $SLEEP_INT
sleep $SLEEP_INT

# Insert client module in DB for deployment to host_id = 2
echo "*************** Deploy Client Module  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "demo_dns_client_app_tag" --host 2

sleep $SLEEP_INT


# start middlebox collection process
echo "*************** Starting Middlebox DCA on Server  ***************"
gnome-terminal -- bash -c  "echo '*************** Starting TCPDUMP  ***************'; tcpdump udp port 53 -i any -w $GIT_DIR/middle_demo.pcap"

sleep $SLEEP_INT


# start dnsmasq process on server
echo "*************** Starting DNSMASQ on Server  ***************"
gnome-terminal -- bash -c  "echo '*************** Starting DNSMASQ  ***************'; dnsmasq --no-daemon -c 0"

sleep $SLEEP_INT


# perform DNS requests from client
# turn off interface to force using local DNS server
echo "*************** Conducting DNS Queries from Client  ***************"
sshpass -p "$CLIENT_PASSWD" ssh -p 22 root@$CLIENT_IP "ifconfig enp0s3 down; sleep 3; dig @10.0.0.20 -p 53 www.dig_test.com; curl www.curl_test.com;"

sleep $SLEEP_INT


echo "*************** finished  ***************"

# Revoke client module in DB host_id = 2
echo "*************** Revoke Client Module   ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "demo_dns_client_app_tag" --host 2

sleep $SLEEP_INT
sleep $SLEEP_INT


# Revoke client module in DB host_id = 2
echo "*************** Revoke Server Module   ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "demo_dns_server_app_tag" --host 1

sleep $SLEEP_INT
sleep $SLEEP_INT


echo "removing layer 4.5"
sshpass -p "$CLIENT_PASSWD" ssh -p 22 root@$CLIENT_IP "pkill python; rmmod layer4_5; ifconfig enp0s3 up"


sleep $SLEEP_INT


pkill tcpdump
pkill dnsmasq
pkill python3

rmmod layer4_5


echo "Opening Wireshark PCAP"
wireshark -r $GIT_DIR/middle_demo.pcap &
