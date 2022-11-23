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

# ----------------------------------------------------------------
# Function for dns test
#   Accepts 1 argument:
#     test name
# ----------------------------------------------------------------
conduct_dns() {
    for ((j = 1; j <= 2; j++)); do
        query="www.test_$1_$j.com"
        dig @$SERVER_IP -p 53 $query >/dev/null
        sleep 0.5
    done
}

echo "Installing Layer 4.5 on server and client"

sshpass -p "$SERVER_PASSWD" ssh -p 22 -o StrictHostKeyChecking=no root@$SERVER_IP "pkill python3; rmmod layer4_5; systemctl stop dnsmasq.service; systemctl stop systemd-resolved.service; $DCA_KERNEL_DIR/bash/installer.sh; dnsmasq --no-daemon -c 0 >/dev/null 2>&1 &"

sleep 1

rmmod layer4_5

$DCA_KERNEL_DIR/bash/installer.sh

sleep 2

# start NCO process with command line params
echo "*************** Starting NCO on Client  ***************"
gnome-terminal -- bash -c "echo '*************** Starting NCO  ***************'; python3 $NCO_DIR/NCO.py --query_interval 5 --build_interval 3 --linear --print --ip $CLIENT_IP --logfile $NCO_DIR/priority_nco_messages.log"

sleep 3

# start client DCA process, which will have host_id = 1
echo "*************** Starting DCA on Client  ***************"
gnome-terminal -- bash -c "echo '*************** Client DCA  ***************'; python3 $DCA_USER_DIR/DCA.py --print --logfile $DCA_USER_DIR/priority_client_dca_messages.log --ip $CLIENT_IP"

sleep 5

# Insert client module in DB for deployment to host_id = 1, with active mode set to true
echo "*************** Deploy Client Module High  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "dns_client_high_priority" --host 1 --priority 42 --activate

sleep 5

echo "*************** Deploy Client Module Low ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "dns_client_low_priority" --host 1 --priority 100 --activate

sleep 7

# start DCA process on server, which will have host_id = 2
echo "*************** Starting DCA on Server  ***************"
sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "python3 $DCA_USER_DIR/DCA.py --ip $CLIENT_IP --logfile $DCA_USER_DIR/priority_server_dca_messages.log  >/dev/null 2>&1 &"

sleep 5

# Insert server module in DB for deployment to host_id = 2, with active mode set to true and with applyNow set
echo "*************** Deploy Server Module Low  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "dns_server_low_priority" --host 2 --priority 99 --applyNow --activate

sleep 7

# high after low so we test re-sorting
echo "*************** Deploy Server Module High  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "dns_server_high_priority" --host 2 --priority 12 --applyNow --activate

sleep 5

# start middlebox collection process
echo "*************** Starting Middlebox DCA on Client  ***************"
gnome-terminal -- bash -c "echo '*************** Starting TCPDUMP  ***************'; tcpdump port 53 -i any -w $GIT_DIR/priority.pcap"

sleep 10

echo "*************** starting High test ***************"

conduct_dns "high"

echo "*************** finished High test ***************"

# Now we change the server priority level of High to 120 and ensure still process high cust
echo "*************** Change Server High Priority ***************"
python3 $NCO_DIR/priority_module_helper.py --module "dns_server_high_priority" --host 2 --priority 120

sleep 15

echo "*************** starting High test 2 ***************"

conduct_dns "high_still"

echo "*************** finished High test 2 ***************"

# Now we change the client priority level of High to 222 and ensure low is sent and processed
echo "*************** Change Client High Priority ***************"
python3 $NCO_DIR/priority_module_helper.py --module "dns_client_high_priority" --host 1 --priority 222

sleep 15 # give plenty of time for module to reflect new active status

echo "*************** starting Low test ***************"

conduct_dns "low"

echo "*************** finished Low test ***************"

sleep 2

# now make sure we can revoke the high module and only have low applied to both sides

# Revoke server high module in DB host_id = 2
echo "*************** Revoke Server High Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "dns_server_high_priority" --host 2

sleep 2

# Revoke client high module in DB host_id = 1
echo "*************** Revoke Client High Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "dns_client_high_priority" --host 1

sleep 15

echo "*************** starting Low test 2 ***************"

conduct_dns "low_still"

echo "*************** finished Low test 2 ***************"

# Revoke server high module in DB host_id = 2
echo "*************** Revoke Server Low Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "dns_server_low_priority" --host 2

sleep 2

# Revoke client high module in DB host_id = 1
echo "*************** Revoke Client Low Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "dns_client_low_priority" --host 1

sleep 15

echo "*************** starting no cust test ***************"

conduct_dns "no_cust"

echo "*************** finished no cust test ***************"

sleep 2

echo "cleaning up"

# save tracelog files from each host
sudo cp /sys/kernel/tracing/trace $EXP_SCRIPT_DIR/logs/priority_client.txt

rmmod layer4_5
pkill tcpdump

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill python3; pkill dnsmasq; systemctl start systemd-resolved.service; sudo cp /sys/kernel/tracing/trace $EXP_SCRIPT_DIR/logs/priority_server.txt; rmmod layer4_5"

pkill python3

# TODO: run a python file to determine success/failure of test automatically
# parse log files and pcap
# priority_*.txt: identify NLMSG and subsequent L4.5 call, insmod with app name and prio levels
# parse pcap: high tag twice, low tag twice, no tag
