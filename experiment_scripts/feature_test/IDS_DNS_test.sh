#!/bin/bash

# Purpose: test the automated functionality of the NCO to defeat DNS Flood attack
# 
# security check window = 5
# standard report interval = 8
# wait 20 seconds before deprecating module
# wait additional 30 seconds before finishing test module
# build window = 3 to speed up deploying module

#sec_window=5
report_window=5
first_wait=20
finish_wait=30

# ************** STANDARD PARAMS MUST GO HERE ****************
GIT_DIR=/home/vagrant/software_defined_customization
NCO_DIR=/home/vagrant/software_defined_customization/NCO
EXP_SCRIPT_DIR=/home/vagrant/software_defined_customization/experiment_scripts
#NETSOFT_SCRIPT_DIR=/home/vagrant/software_defined_customization/experiment_scripts/netsoft
#GENI_SCRIPT_DIR=/home/vagrant/software_defined_customization/experiment_scripts/geni
#LAYER_MOD_DIR=/home/vagrant/software_defined_customization/layer4_5_modules
#NETSOFT_MOD_DIR=/home/vagrant/software_defined_customization/layer4_5_modules/netsoft
#GENI_MOD_DIR=/home/vagrant/software_defined_customization/layer4_5_modules/geni
IDS_MOD_DIR=/home/vagrant/software_defined_customization/layer4_5_modules/IDS_modules
#SIMPLE_SERVER_DIR=/home/vagrant/software_defined_customization/experiment_scripts/client_server
DCA_KERNEL_DIR=/home/vagrant/software_defined_customization/DCA_kernel
DCA_USER_DIR=/home/vagrant/software_defined_customization/DCA_user
CUST_LOCATION=/usr/lib/modules/$distro/layer4_5/customizations
SERVER_IP=192.168.0.18
SERVER_PASSWD=vagrant
CLIENT1_IP=192.168.0.19
CLIENT2_IP=192.168.0.12
CLIENT3_IP=192.168.0.13
CLIENT_PASSWD=vagrant
# ************** END STANDARD PARAMS  ****************

# Force root
if [[ "$(id -u)" != "0" ]]; then
    echo "This script must be run as root" 1>&2
    exit -1
fi

#install Layer 4.5 on device
echo "*************** cib removed  ***************"

#ensure layer4.5 is not currently running and DB doesn't exist
#rmmod layer4_5
rm $NCO_DIR/cib.db

#$DCA_KERNEL_DIR/bash/installer.sh
# start NCO process with command line params
echo "*************** Starting NCO  ***************"
gnome-terminal -- bash -c "echo '*************** Starting NCO  ***************';  python3 $NCO_DIR/NCO.py --query_interval $report_window --print --logfile $NCO_DIR/IDS_DNS.log"

echo "*************** Starting DNS Server ***************"
gnome-terminal -- bash -c "cd $IDS_MOD_DIR; python3 dns_server.py"
sleep 5

# start DCA process
echo "*************** Starting Client 1 DCA  ***************"
gnome-terminal -- bash -c "echo '*************** Starting DCA 1  ***************'; sshpass -p $CLIENT_PASSWD ssh -o StrictHostKeyChecking=no vagrant@$CLIENT1_IP 'python3 $DCA_USER_DIR/DCA.py --ip $SERVER_IP --print --logfile $DCA_USER_DIR/IDS_DNS_DCA1.log'"

sleep 3

echo "*************** Starting Client 2 DCA  ***************"
gnome-terminal -- bash -c "echo '*************** Starting DCA 2  ***************'; sshpass -p $CLIENT_PASSWD ssh -o StrictHostKeyChecking=no vagrant@$CLIENT2_IP 'python3 $DCA_USER_DIR/DCA.py --ip $SERVER_IP --print --logfile $DCA_USER_DIR/IDS_DNS_DCA2.log'"

sleep 3

echo "*************** Starting Client 3 DCA  ***************"
gnome-terminal -- bash -c "echo '*************** Starting DCA 3  ***************'; sshpass -p $CLIENT_PASSWD ssh -o StrictHostKeyChecking=no vagrant@$CLIENT3_IP 'python3 $DCA_USER_DIR/DCA.py --ip $SERVER_IP --print --logfile $DCA_USER_DIR/IDS_DNS_DCA3.log'"

echo "*************** Allowing NCO -> DCA sync ***************"
sleep 60


# start DNS malware on host 1
echo "*************** Starting DNS Malware on ALL Clients ***************"

sleep 3

gnome-terminal -- bash -c "echo '*************** Malware DCA 1  ***************'; sshpass -p $CLIENT_PASSWD ssh -o StrictHostKeyChecking=no vagrant@$CLIENT1_IP 'python3 $IDS_MOD_DIR/dns_query.py'"
gnome-terminal -- bash -c "echo '*************** Malware DCA 2  ***************'; sshpass -p $CLIENT_PASSWD ssh -o StrictHostKeyChecking=no vagrant@$CLIENT2_IP 'python3 $IDS_MOD_DIR/dns_query.py'"
gnome-terminal -- bash -c "echo '*************** Malware DCA 3  ***************'; sshpass -p $CLIENT_PASSWD ssh -o StrictHostKeyChecking=no vagrant@$CLIENT3_IP 'python3 $IDS_MOD_DIR/dns_query.py'"

echo "*************** DNS Flood initiated ***************"

sleep 5
echo "*************** SLEEP Period 2 minutes ***************"
sleep 120

echo "*************** Verify Hosts/Logs ***************"

# save tracelog files from each host
#sudo cp /sys/kernel/tracing/trace $EXP_SCRIPT_DIR/logs/challenge_deprecate.txt

#pkill python3
#rmmod layer4_5
