#!/bin/bash

#Purpose: attach module, deprecate it and ensure stay active on current socket but not on new sockets

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
# ************** END STANDARD PARAMS  ****************

# Force root
if [[ "$(id -u)" != "0" ]]; then
  echo "This script must be run as root" 1>&2
  exit -1
fi

# ----------------------------------------------------------------
# Function for dns test
#   Accepts 1 argument:
#     test name
# ----------------------------------------------------------------
conduct_dns() {
  for ((j = 1; j <= 5; j++)); do
    query="www.test_$1_$j.com"
    dig @$SERVER_IP -p 53 $query >/dev/null
    sleep 0.5
  done
}

echo "Installing Layer 4.5 on server and client"

sshpass -p "$SERVER_PASSWD" ssh -p 22 -o StrictHostKeyChecking=no root@$SERVER_IP "rmmod layer4_5; systemctl stop dnsmasq.service; systemctl stop systemd-resolved.service; $DCA_KERNEL_DIR/bash/installer.sh"

sleep 1

rmmod layer4_5

$DCA_KERNEL_DIR/bash/installer.sh

sleep 2

# start NCO process with command line params
echo "*************** Starting NCO on Client  ***************"
gnome-terminal -- bash -c "echo '*************** Starting NCO  ***************'; python3 $NCO_DIR/NCO.py --query_interval 5 --build_interval 3 --linear --print --ip $CLIENT_IP --logfile $NCO_DIR/deprecate_nco_messages.log"

sleep 5

# start client DCA process, which will have host_id = 1
echo "*************** Starting DCA on Client  ***************"
gnome-terminal -- bash -c "echo '*************** Client DCA  ***************'; python3 $DCA_USER_DIR/DCA.py --print --logfile $DCA_USER_DIR/client_dca_messages.log --ip $CLIENT_IP"

sleep 5

# start DCA process on server, which will have host_id = 2
echo "*************** Starting DCA on Server  ***************"
sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill dnsmasq; python3 $DCA_USER_DIR/DCA.py --ip $CLIENT_IP --logfile $DCA_USER_DIR/server_dca_messages.log >/dev/null 2>&1 &"

sleep 2

# Insert server module in DB for deployment to host_id = 2
echo "*************** Deploy Server Module  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "demo_dns_server_app_tag" --host 2 --priority 42 --activate

sleep 5

# Insert client module in DB for deployment to host_id = 1
echo "*************** Deploy Client Module  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "demo_dns_client_app_tag" --host 1 --priority 42 --activate

sleep 10

echo "*************** Starting DNSMASQ on Server  ***************"
sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "dnsmasq --no-daemon -c 0 >/dev/null 2>&1 &"

sleep 2

# start middlebox collection process
echo "*************** Starting Middlebox DCA on Client  ***************"
gnome-terminal -- bash -c "echo '*************** Starting TCPDUMP  ***************'; tcpdump port 53 -i any -w $GIT_DIR/deprecate.pcap"

sleep 5

echo "*************** starting cust both ***************"

conduct_dns "both"

# for ((j = 1; j <= 5; j++)); do
#   query="www.test_both$j.com"
#   dig @$SERVER_IP -p 53 $query >/dev/null
#   sleep 0.5
# done

echo "*************** finished cust both ***************"

# deprecate server module in DB host_id = 2
echo "*************** Deprecate Server Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "demo_dns_server_app_tag" --host 2 --deprecate

sleep 15

# dnsmasq still has deprecated module attached, so these should work fine
echo "*************** starting deprecated test ***************"

conduct_dns "deprecate"

# for ((j = 1; j <= 5; j++)); do
#   query="www.test_deprecate$j.com"
#   dig @$SERVER_IP -p 53 $query >/dev/null
#   sleep 0.5
# done

echo "*************** finished deprecated test  ***************"

# deprecate client module in DB host_id = 1
echo "*************** Deprecate Client Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "demo_dns_client_app_tag" --host 1 --deprecate

sleep 15

# now we restart dnsmasq, which should not get the module attached since deprecated
echo "*************** Restarting DNSMASQ on Server  ***************"
sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill dnsmasq; dnsmasq --no-daemon -c 0 >/dev/null 2>&1 &"

sleep 2

echo "*************** starting no module test ***************"

conduct_dns "none"

echo "*************** finished no module test  ***************"

# Revoke server module in DB host_id = 2
echo "*************** Revoke Server Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "demo_dns_server_app_tag" --host 2

sleep 2

# Revoke client module in DB host_id = 1
echo "*************** Revoke Client Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "demo_dns_client_app_tag" --host 1

sleep 15

echo "cleaning up and saving logs"

# save tracelog files from each host
sudo cp /sys/kernel/tracing/trace $EXP_SCRIPT_DIR/logs/deprecate_client.txt

rmmod layer4_5
pkill tcpdump

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill python3; pkill dnsmasq; systemctl start systemd-resolved.service; sudo cp /sys/kernel/tracing/trace $EXP_SCRIPT_DIR/logs/deprecate_server.txt; rmmod layer4_5"

pkill python3

# TODO: run a python file to determine success/failure of test automatically
# parse log files and pcap
# deprecate_*.txt: identify deprecate NLMSG and subsequent L4.5 call, insmod with app name
# parse pcap: ID tag in first 10 dns request, and no tag in last 5
