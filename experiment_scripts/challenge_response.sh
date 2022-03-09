#!/bin/bash

#Purpose: perform the NCO/DCA module deployment experiment
# $1 = security check window (ex: 5)
# $2 = standard report interval (ex: 5)
# $3 = wait X seconds before cleaning up (ex: 60)

#directory holding the software_defined_customization git repo
GIT_DIR=/home/dan/software_defined_customization


#install Layer 4.5 on device
echo "*************** Install L4.5  ***************"

#ensure layer4.5 is not currently running and DB doesn't exist
rmmod layer4_5
rm $GIT_DIR/NCO/cib.db

$GIT_DIR/DCA_kernel/bash/installer.sh

sleep 2

# start NCO process with command line params
echo "*************** Starting NCO  ***************"
gnome-terminal -- python3 $GIT_DIR/NCO/NCO.py --challenge --window $1 --query_interval $2 --linear > $GIT_DIR/NCO/challenge\_NCO\_output.txt

sleep 2

# start DCA process, which will have host_id = 1
echo "*************** Starting DCA  ***************"
gnome-terminal --  python3 $GIT_DIR/DCA\_user/DCA.py > $GIT_DIR/DCA\_user/challenge\_DCA\_output.txt


sleep 2

# Insert challenge module in DB for deployment to host_id = 1
echo "*************** Deploy Module  ***************"
python3 $GIT_DIR/NCO/deploy_module_helper.py --module "nco_challenge_response" --host 1


sleep $3


echo "*************** finished  ***************"


echo "removing challenge module"

rmmod nco_challenge_response

sleep $2
