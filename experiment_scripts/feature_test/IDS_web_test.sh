#!/bin/bash

# Purpose: perform DCA module alert test with string detection in socket buffer
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
#GIT_DIR=/home/vagrant/software_defined_customization
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
CLIENT_IP=192.168.0.19
CLIENT_PASSWD=vagrant
# ************** END STANDARD PARAMS  ****************

# Force root
if [[ "$(id -u)" != "0" ]]; then
    echo "This script must be run as root" 1>&2
    exit -1
fi

#install Layer 4.5 on device
#echo "*************** Install L4.5  ***************"

#ensure layer4.5 is not currently running and DB doesn't exist
#rmmod layer4_5
#rm $NCO_DIR/cib.db

#$DCA_KERNEL_DIR/bash/installer.sh

#sleep 2

# start NCO process with command line params

#echo "*************** Starting NCO  ***************"
#gnome-terminal -- bash -c "echo '*************** Starting NCO  ***************';  python3 $NCO_DIR/NCO.py --build_interval 3 --challenge --window $sec_window --query_interval $report_window --linear --print --logfile $NCO_DIR/IDS_web_test.log"

#sleep 2

# start DCA process
echo "*************** Starting DCA ***************"
gnome-terminal -- bash -c "echo '*************** Starting DCA ***************'; python3 $DCA_USER_DIR/DCA.py --print --logfile $DCA_USER_DIR/IDS_web_test.log; exec bash"

echo "*************** Inserting IDS Module  ***************"
gnome-terminal -- bash -c "cyclelog; tail -f /sys/kernel/tracing/trace; exec bash"
gnome-terminal -- bash -c "cd $IDS_MOD_DIR && ./redo.sh IDS_web_client; exit"


echo "*************** Sleeping for Cust_report send ***************"
sleep 10

echo "*************** Sending Malicious web request ***************"
gnome-terminal -- bash -c "curl http://localhost:8080/$IDS_MOD_DIR/exampleurl.com;"

echo "*************** Sleeping for Cust_report send ***************"
sleep 10

echo "*************** Waiting for NCO Module ***************"
sleep 10







#sshpass -p "$CLIENT_PASSWD" ssh -p 22 root@$CLIENT_IP "tmux new-session -d -s mysession 'bash'"
#sshpass -p "$CLIENT_PASSWD" ssh -p 22 root@$CLIENT_IP "screen -dmS mysession bash -c 'cd $DCA_USER_DIR && sudo python3 ./DCA.py --print --logfile $DCA_USER_DIR/web_test_DCA.log'"


#sshpass -p "$CLIENT_PASSWD" ssh -p 22 -o StrictHostKeyChecking=no root@$CLIENT_IP \
    
    #"gnome-terminal -- bash -c \"cd '$DCA_USER_DIR' && sudo python3 ./DCA.py --print --logfile '$DCA_USER_DIR/web_test_DCA.log'; exec bash\" & \
     #sleep 2; \
     #gnome-terminal -- bash -c \"cd '$IDS_MOD_DIR' && ./redo.sh IDS_web_client; exit\" &"


# insert IDS module
#echo "*************** Inserting IDS Module  ***************"
#sshpass -p "$CLIENT_PASSWD" ssh -p 22 -o StrictHostKeyChecking=no root@$CLIENT_IP \
#    "gnome-terminal -- bash -c 'cd $IDS_MOD_DIR && ./redo.sh IDS_web_client; exit' &"

sleep $finish_wait
echo "*************** Finished ***************"

# Note: challenge/response still happening for deprecated module since still active on the socket




#sleep $report_window
#sleep $report_window

# save tracelog files from each host
#sudo cp /sys/kernel/tracing/trace $EXP_SCRIPT_DIR/logs/challenge_deprecate.txt

#pkill python3
#rmmod layer4_5

# TODO: run a python file to determine success/failure of test automatically
# parse log files
# challenge_*.txt: identify challenge NLMSG and subsequent L4.5 call, insmod with app name
# NCO/DCA logs: verify challenge passed each time, no failures; deprecate transition
