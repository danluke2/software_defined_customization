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

#start tcpdump on dns server
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $SERVER_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$SERVER "sudo tcpdump udp port 53 -i any -w $GIT_DIR/rotate_pair_exp.pcap &"

sleep 2

#deploy DNS server and H1 tagging module in active state
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $NCO_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$NCO "cd $NCO_DIR; sudo python3 deploy_module_helper.py --module rotate_dns_server_front --host 3 --activate --priority 10 --applyNow; sudo python3 deploy_module_helper.py --module rotate_host_front --host 1 --activate --priority 10 --applyNow"

sleep 20

#start H1 DNS requests to DNS server
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $HOST_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$HOST "cd $GENI_SCRIPT_DIR; ./immediate_dns_host_repeat.sh 20 10.10.0.3 &"

sleep 5

#deploy web server and H1 compression module in active state with lower priority and immediate attachment
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $NCO_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$NCO "cd $NCO_DIR; sudo python3 deploy_module_helper.py --module rotate_dns_server_compress --host 3 --activate --priority 20 --applyNow; sudo python3 deploy_module_helper.py --module rotate_host_compress --host 1 --activate --priority 20 --applyNow"

sleep 10

#update priority of server tagging module
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $NCO_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$NCO "cd $NCO_DIR; sudo python3 priority_module_helper.py --module rotate_dns_server_front --host 3  --priority 30"

sleep 10

#update priority of host tagging module
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $NCO_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$NCO "cd $NCO_DIR; sudo python3 priority_module_helper.py --module rotate_host_front --host 1  --priority 30"

sleep 10

#revoke all modules
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $NCO_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$NCO "cd $NCO_DIR; sudo python3 revoke_module_helper.py --module rotate_dns_server_front --host 3; sudo python3 revoke_module_helper.py --module rotate_dns_server_compress --host 3; sudo python3 revoke_module_helper.py --module rotate_host_compress --host 1; sudo python3 revoke_module_helper.py --module rotate_host_front --host 1"

sleep 15

#stop tcpdump on web server, make log files
sshpass -P passphrase -p "$PASSWORD" ssh -t -p $SERVER_PORT -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$SERVER "sudo pkill tcpdump; sudo cp /sys/kernel/tracing/trace $GIT_DIR/rotate_pair_tracelog.txt "
