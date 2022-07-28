#!/bin/bash

# Purpose: attach two module with different priority levels and ensure both attach to sockets
# Then we send a priority update command to change which module should be applied

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

echo "Installing Layer 4.5 on server and client"

sshpass -p "$SERVER_PASSWD" ssh -p 22 -o StrictHostKeyChecking=no root@$SERVER_IP "pkill python3; rmmod layer4_5; $DCA_KERNEL_DIR/bash/installer.sh; $SIMPLE_SERVER_DIR/service.sh; systemctl restart simple_server.service"

sleep 1

rmmod layer4_5

$DCA_KERNEL_DIR/bash/installer.sh

sleep 2

# start NCO process with command line params
echo "*************** Starting NCO on Client  ***************"
gnome-terminal -- bash -c "echo '*************** Starting NCO  ***************'; python3 $NCO_DIR/NCO.py --query_interval 5 --build_interval 3 --linear --print --ip $CLIENT_IP --logfile $NCO_DIR/priority_stream_single_nco_messages.log"

sleep 3

# start client DCA process, which will have host_id = 1
echo "*************** Starting DCA on Client  ***************"
gnome-terminal -- bash -c "echo '*************** Client DCA  ***************'; python3 $DCA_USER_DIR/DCA.py --print --logfile $DCA_USER_DIR/priority_stream_single_client_dca_messages.log --ip $CLIENT_IP"

sleep 5

# start DCA process on server, which will have host_id = 2
echo "*************** Starting DCA on Server  ***************"
sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "python3 $DCA_USER_DIR/DCA.py --ip $CLIENT_IP --logfile $DCA_USER_DIR/priority_stream_single_server_dca_messages.log  >/dev/null 2>&1 &"

sleep 5

# Insert server module in DB for deployment to host_id = 2, with active mode set to true and with applyNow set
echo "*************** Deploy Server Module High  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "deprecate_web_server" --host 2 --priority 10 --applyNow --activate

sleep 7

# insert lower pri module
echo "*************** Deploy Server Module Low  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "activate_web_server" --host 2 --priority 20 --applyNow --activate

sleep 5

# start middlebox collection process
echo "*************** Starting Middlebox DCA on Client  ***************"
gnome-terminal -- bash -c "echo '*************** Starting TCPDUMP  ***************'; tcpdump port 8080 -i any -w $GIT_DIR/priority_stream_single.pcap"

sleep 10

echo "*************** starting 1000msec test ***************"

curl http://$SERVER_IP:8080/$GENI_MOD_DIR/layer4_5.ko -o test.ko

echo "*************** finished 1000msec test ***************"

# Now we change the server priority level of low to 5 and ensure order changes
echo "*************** Change Server Low Priority ***************"
python3 $NCO_DIR/priority_module_helper.py --module "activate_web_server" --host 2 --priority 5

sleep 15

echo "*************** starting 100msec test ***************"

curl http://$SERVER_IP:8080/$GENI_MOD_DIR/layer4_5.ko -o test_low.ko

echo "*************** finished 100msec test ***************"

# Revoke server activate module in DB host_id = 2
echo "*************** Revoke Server High Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "activate_web_server" --host 2

sleep 15

echo "*************** starting 1000msec test ***************"

curl http://$SERVER_IP:8080/$GENI_MOD_DIR/layer4_5.ko -o test_1000.ko

echo "*************** finished 1000msec test ***************"

# Revoke server 1000msec module in DB host_id = 2
echo "*************** Revoke Server Low Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "deprecate_web_server" --host 2

sleep 15

echo "*************** starting no cust test ***************"

curl http://$SERVER_IP:8080/$GENI_MOD_DIR/layer4_5.ko -o test_none.ko

echo "*************** finished no cust test ***************"

sleep 2

echo "cleaning up"

# save tracelog files from each host
sudo cp /sys/kernel/tracing/trace $EXP_SCRIPT_DIR/logs/priority_stream_single_client.txt

rmmod layer4_5
pkill tcpdump

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill python3; sudo cp /sys/kernel/tracing/trace $EXP_SCRIPT_DIR/logs/priority_stream_single_server.txt; rmmod layer4_5"

pkill python3

# TODO: run a python file to determine success/failure of test automatically
# parse log files and pcap
# priority_*.txt: identify NLMSG and subsequent L4.5 call, insmod with app name and prio levels
# parse pcap: high tag twice, low tag twice, no tag
