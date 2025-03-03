#!/bin/bash

#Purpose: perform the NCO/DCA module deployment experiment
# $1 = security check window (ex: 5)
# $2 = standard report interval (ex: 5)
# $3 = wait X seconds before cleaning up (ex: 60)

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
SIMPLE_SERVER_DIR=/home/vagrant/software_defined_customization/experiment_scripts/client_server
DCA_KERNEL_DIR=/home/vagrant/software_defined_customization/DCA_kernel
DCA_USER_DIR=/home/vagrant/software_defined_customization/DCA_user
CUST_LOCATION=/usr/lib/modules/$distro/layer4_5/customizations
SERVER_IP=10.0.0.20
SERVER_PASSWD=vagrant
CLIENT_IP=10.0.0.30
CLIENT_PASSWD=vagrant
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
gnome-terminal -- bash -c "echo '*************** Starting NCO  ***************';  python3 $NCO_DIR/NCO.py --challenge --window $1 --query_interval $2 --linear --print"

sleep 2

# start DCA process, which will have host_id = 1
echo "*************** Starting DCA  ***************"
gnome-terminal -- bash -c "echo '*************** Starting DCA  ***************';  python3 $DCA_USER_DIR/DCA.py --print"

sleep 2

# Insert challenge module in DB for deployment to host_id = 1
echo "*************** Deploy Module  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "nco_challenge_response" --host 1

sleep $3

echo "*************** finished  ***************"

# Revoke client module in DB host_id = 1
echo "*************** Revoke Challenge Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "nco_challenge_response" --host 1

sleep $3
