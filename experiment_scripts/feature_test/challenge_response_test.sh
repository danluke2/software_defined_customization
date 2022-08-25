#!/bin/bash

# Purpose: perform the NCO/DCA module deployment experiment with deprecation testing
# Module inserted in non-activated status since we are just testing challenge capability
# security check window = 5
# standard report interval = 8
# wait 20 seconds before deprecating module
# wait additional 30 seconds before finishing test module
# build window = 3 to speed up deploying module

sec_window=5
report_window=8
first_wait=20
finish_wait=30

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
CUST_LOCATION=/usr/lib/modules/5.15.0-46-generic/layer4_5/customizations
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

#install Layer 4.5 on device
echo "*************** Install L4.5  ***************"

#ensure layer4.5 is not currently running and DB doesn't exist
rmmod layer4_5
rm $NCO_DIR/cib.db

$DCA_KERNEL_DIR/bash/installer.sh

sleep 2

# start NCO process with command line params
echo "*************** Starting NCO  ***************"
gnome-terminal -- bash -c "echo '*************** Starting NCO  ***************';  python3 $NCO_DIR/NCO.py --build_interval 3 --challenge --window $sec_window --query_interval $report_window --linear --print --logfile $NCO_DIR/challenge_nco_messages.log"

sleep 2

# start DCA process, which will have host_id = 1
echo "*************** Starting DCA  ***************"
gnome-terminal -- bash -c "echo '*************** Starting DCA  ***************';  python3 $DCA_USER_DIR/DCA.py --print --logfile $DCA_USER_DIR/challenge_dca_messages.log"

sleep $report_window

# Insert challenge module in DB for deployment to host_id = 1
echo "*************** Deploy Module  ***************"
# python3 $NCO_DIR/deploy_module_helper.py --module "nco_challenge_response" --host 1
python3 $NCO_DIR/deploy_module_helper.py --module "demo_dns_server_app_tag" --host 1

sleep $first_wait

echo "*************** Finished Normal Part  ***************"

# deprecate client module in DB host_id = 2
echo "*************** Deprecate Challenge Module ***************"
# python3 $NCO_DIR/revoke_module_helper.py --module "nco_challenge_response" --host 1 --deprecate
python3 $NCO_DIR/revoke_module_helper.py --module "demo_dns_server_app_tag" --host 1 --deprecate

# Note: challenge/response still happening for deprecated module since still active on the socket

sleep $finish_wait

# Revoke client module in DB host_id = 2
echo "*************** Revoke Challenge Module ***************"
# python3 $NCO_DIR/revoke_module_helper.py --module "nco_challenge_response" --host 1
python3 $NCO_DIR/revoke_module_helper.py --module "demo_dns_server_app_tag" --host 1

sleep $report_window
sleep $report_window

# save tracelog files from each host
sudo cp /sys/kernel/tracing/trace $EXP_SCRIPT_DIR/logs/challenge_deprecate.txt

pkill python3
rmmod layer4_5

# TODO: run a python file to determine success/failure of test automatically
# parse log files
# challenge_*.txt: identify challenge NLMSG and subsequent L4.5 call, insmod with app name
# NCO/DCA logs: verify challenge passed each time, no failures; deprecate transition
