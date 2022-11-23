#!/bin/bash

#Purpose: perform dns requests to test immediate cust attachment
# $1 = DNS server to connect to
# $2 = DNS machine port number to attach to
# $3 = host machine to connect to
# $4 = host port number
# $5 = NCO
# $6 = NCO port

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
NCO=$5
NCO_PORT=$6

#start tcpdump on DNS server
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $SERVER_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$SERVER "sudo tcpdump udp port 53 -i any -w $GIT_DIR/immediate_exp.pcap &"

sleep 2

#start H1 DNS requests to DNS server
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $HOST_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$HOST "cd $GENI_SCRIPT_DIR; ./immediate_dns_host_repeat.sh 10 10.10.0.3 &"

sleep 5

#deploy DNS server module
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $NCO_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$NCO "cd $NCO_DIR; sudo python3 deploy_module_helper.py --module immediate_dns_server --host 3 --activate --applyNow --priority 1"

sleep 30

#revoke DNS server module
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $NCO_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$NCO "cd $NCO_DIR; sudo python3 revoke_module_helper.py --module immediate_dns_server --host 3"

sleep 15

#stop tcpdump on DNS server, make log files
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $SERVER_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$SERVER "sudo pkill tcpdump; sudo cp /sys/kernel/tracing/trace $GIT_DIR/immediate_tracelog.txt "
