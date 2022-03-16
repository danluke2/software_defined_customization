#!/bin/bash

#Purpose: perform the NCO/DCA module deployment experiment
# $1 = security check window (ex: 5)
# $2 = standard report interval (ex: 5)
# $3 = wait X seconds before cleaning up (ex: 60)


# ************** STANDARD PARAMS MUST GO HERE ****************
GIT_DIR=/home/vagrant/software_defined_customization
NCO_DIR=/home/vagrant/software_defined_customization/NCO
EXP_SCRIPT_DIR=/home/vagrant/software_defined_customization/experiment_scripts
EXP_MOD_DIR=/home/vagrant/software_defined_customization/experiment_modules
SIMPLE_SERVER_DIR=/home/vagrant/software_defined_customization/experiment_scripts/client_server
DCA_KERNEL_DIR=/home/vagrant/software_defined_customization/DCA_kernel
DCA_USER_DIR=/home/vagrant/software_defined_customization/DCA_user

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


#install Layer 4.5 on device
echo "*************** Install L4.5  ***************"

#ensure layer4.5 is not currently running and DB doesn't exist
rmmod layer4_5
rm $NCO_DIR/cib.db

$DCA_KERNEL_DIR/bash/installer.sh

sleep 2

# start NCO process with command line params
echo "*************** Starting NCO  ***************"
gnome-terminal -- python3 $NCO_DIR/NCO.py --challenge --window $1 --query_interval $2 --linear

sleep 2

# start DCA process, which will have host_id = 1
echo "*************** Starting DCA  ***************"
gnome-terminal --  python3 $DCA_USER_DIR/DCA.py


sleep 2

# Insert challenge module in DB for deployment to host_id = 1
echo "*************** Deploy Module  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "nco_challenge_response" --host 1


sleep $3


echo "*************** finished  ***************"


echo "removing challenge module"

rmmod nco_challenge_response

sleep $2
