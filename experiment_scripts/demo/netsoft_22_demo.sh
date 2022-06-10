#!/bin/bash

#Purpose: Perform multiple customizations to demo Layer 4.5 at NetSoft 22

# 1) Instal front DNS module on client and server with inverse module, challenge set at 10 sec

# 2) Install end DNS module on client only with inverse module, challenge set at 10 sec

# Run on client VM

sec_window=12
report_window=7

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
    for ((j = 1; j <= 3; j++)); do
        query="www.test_$1_$j.com"
        echo "$query"
        dig @$SERVER_IP -p 53 $query >/dev/null
        sleep 2
    done
}

echo "Installing Layer 4.5 on server and client"

sshpass -p "$SERVER_PASSWD" ssh -p 22 -o StrictHostKeyChecking=no root@$SERVER_IP "rmmod layer4_5; systemctl stop dnsmasq.service; systemctl stop systemd-resolved.service; $DCA_KERNEL_DIR/bash/installer.sh"

sleep 1

rmmod layer4_5

$DCA_KERNEL_DIR/bash/installer.sh

sleep 2

# start NCO process with command line params
echo
echo "*************** Starting NCO on Client  ***************"
gnome-terminal --window-with-profile=demo -- bash -c "echo '*************** Starting NCO  ***************'; python3 $NCO_DIR/NCO.py --challenge --window $sec_window --query_interval $report_window --build_interval 5 --linear --print --ip $CLIENT_IP --logfile $NCO_DIR/demo_messages.log"

sleep 10

# start client DCA process, which will have host_id = 1
echo
echo "*************** Starting DCA on Client/Server  ***************"
gnome-terminal --window-with-profile=demo -- bash -c "echo '*************** Client DCA  ***************'; python3 $DCA_USER_DIR/DCA.py --print --logfile $DCA_USER_DIR/client_demo_messages.log --ip $CLIENT_IP"

sleep 5

# start DCA process on server, which will have host_id = 2
# echo "*************** Starting DCA on Server  ***************"
sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill dnsmasq; python3 $DCA_USER_DIR/DCA.py --ip $CLIENT_IP --logfile $DCA_USER_DIR/server_demo_messages.log >/dev/null 2>&1 &"

sleep 5

# start DCA middlebox process
echo
echo "*************** Starting Middlebox DCA on Server  ***************"
sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "python3 $DCA_USER_DIR/DCA_middlebox.py --print --ip 10.0.0.40 --logfile $DCA_USER_DIR/middlebox_demo_messages.log >/dev/null 2>&1 &"

sleep 5

# Insert server module in DB for deployment to host_id = 2
echo
echo "*************** Deploy Server DNS Front Module and Inverse  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "demo_dns_server_app_tag" --host 2 --activate --inverse "demo_dns_tag_both.lua" --type "Wireshark"

sleep 15

# Insert client module in DB for deployment to host_id = 1
echo "*************** Deploy Client DNS Front Module  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "demo_dns_client_app_tag" --host 1 --activate

sleep 10

# echo "*************** Starting DNSMASQ on Server  ***************"
sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "dnsmasq --no-daemon -c 0 >/dev/null 2>&1 &"

sleep 1

echo
echo "*************** Starting TCPDUMP on Server  ***************"
sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "tcpdump udp port 53 -i enp0s8 -w $GIT_DIR/netsoft_demo.pcap >/dev/null 2>&1 &"

sleep 5

echo "*************** starting DNS traffic ***************"

conduct_dns "both_front"

echo "*************** finished DNS traffic ***************"

# Revoke server module in DB host_id = 2
echo
echo "*************** Revoke DNS Front Modules ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "demo_dns_server_app_tag" --host 2

sleep 1

# Revoke client module in DB host_id = 1
# echo "*************** Revoke Client DNS Front Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "demo_dns_client_app_tag" --host 1

sleep 15

echo
echo "*************** Starting DNS End Tagging from Client ***************"

# Insert client module in DB for deployment to host_id = 1
echo
echo "*************** Deploy Client DNS End Module  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "demo_dns_client_app_end_tag" --host 1 --activate

sleep 10

echo "*************** starting DNS traffic ***************"

conduct_dns "client_end"

echo "*************** finished DNS traffic ***************"

sleep 2

echo
echo "cleaning up and saving logs"

# save tracelog files from each host
sudo cp /sys/kernel/tracing/trace $EXP_SCRIPT_DIR/logs/demo_client.txt

rmmod demo_dns_client_app_end_tag
rmmod layer4_5

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill python3; pkill dnsmasq; pkill tcpdump; systemctl start systemd-resolved.service; sudo cp /sys/kernel/tracing/trace $EXP_SCRIPT_DIR/logs/demo_server.txt; rmmod layer4_5"

pkill python3
