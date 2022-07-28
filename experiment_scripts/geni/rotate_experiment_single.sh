#!/bin/bash

#Purpose: perform file transfers to test cust deprecation
# $1 = Web server to connect to
# $2 = Web machine port number to attach to
# $3 = host machine to connect to
# $4 = host port number
# $5 = host2 machine to connect to
# $6 = host2 port number
# $7 = NCO
# $8 = NCO port

# ************** STANDARD PARAMS MUST GO HERE ****************
GIT_DIR=~/software_defined_customization
NCO_DIR=$GIT_DIR/NCO
EXP_SCRIPT_DIR=$GIT_DIR/experiment_scripts
NETSOFT_SCRIPT_DIR=$GIT_DIR/experiment_scripts/netsoft
GENI_SCRIPT_DIR=$GIT_DIR/experiment_scripts/geni
LAYER_MOD_DIR=$GIT_DIR/layer4_5_modules
NETSOFT_MOD_DIR=$GIT_DIR/layer4_5_modules/netsoft
GENI_MOD_DIR=$GIT_DIR/layer4_5_modules/geni
SIMPLE_SERVER_DIR=$GIT_DIR/experiment_scripts/client_server
DCA_KERNEL_DIR=$GIT_DIR/DCA_kernel
DCA_USER_DIR=$GIT_DIR/DCA_user
USERNAME=dflukasz
PASSWORD=geni_ssh
# ************** END STANDARD PARAMS  ****************

SERVER=$1
SERVER_PORT=$2
HOST=$3
HOST_PORT=$4
HOST2=$5
HOST_PORT2=$6
NCO=$7
NCO_PORT=$8

#start tcpdump on web server
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $SERVER_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$SERVER "sudo tcpdump tcp port 8080 -i any -w $GIT_DIR/rotate_exp.pcap &"

sleep 2

#deploy web server 1000-msec module in active state
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $NCO_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$NCO "cd $NCO_DIR; sudo python3 deploy_module_helper.py --module deprecate_web_server --host 4 --activate --priority 10"

sleep 20

#H1 file request to Web server
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $HOST_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$HOST "curl http://10.10.0.4:8080/users/${USERNAME}/software_defined_customization/experiment_scripts/geni/layer4_5.ko -o test_with_cust.ko &"

sleep 1

#deploy web server 100-msec module in active state with lower priority and immediate attachment
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $NCO_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$NCO "cd $NCO_DIR; sudo python3 deploy_module_helper.py --module activate_web_server --host 4 --activate --priority 20 --applyNow"

sleep 7

#update priority of web server 100-msec module
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $NCO_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$NCO "cd $NCO_DIR; sudo python3 priority_module_helper.py --module activate_web_server --host 4  --priority 5"

sleep 45

#revoke server modules
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $NCO_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$NCO "cd $NCO_DIR; sudo python3 revoke_module_helper.py --module deprecate_web_server --host 4; sudo python3 revoke_module_helper.py --module activate_web_server --host 4"

sleep 15

#stop tcpdump on web server, make log files
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $SERVER_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$SERVER "sudo pkill tcpdump; sudo cp /sys/kernel/tracing/trace $GIT_DIR/rotate_single_tracelog.txt "
