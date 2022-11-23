#!/bin/bash

#Purpose: perform dns requests to test public DNS server end cust
# $1 = number of trials
# $2 = server to connect to
# $3 = client IP address to mach customization

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
USERNAME=
PASSWORD=
# ************** END STANDARD PARAMS  ****************

SERVER_IP=$2
CLIENT_IP=$3

# Force root
if [[ "$(id -u)" != "0" ]]; then
    echo "This script must be run as root" 1>&2
    exit -1
fi

# create file to store query results
OUTPUT=$EXP_SCRIPT_DIR/logs/$SERVER_IP/public_dns_${2}.txt
touch $OUTPUT

echo "$SERVER_IP" >>$OUTPUT
echo "Baseline" >>$OUTPUT

rmmod layer4_5

echo "*************** starting baseline batch ***************"

echo "starting dns requests"

for ((i = 1; i <= $1; i++)); do
    echo "DNS test $i"
    for ((j = 1; j <= 5; j++)); do
        query="www.nps.edu"
        dig +time=5 +tries=2 @$SERVER_IP -p 53 $query | grep -A 2 "ANSWER SECTION" >>$OUTPUT
        sleep 2
    done
done

echo "*************** finished baseline test ***************"

echo "*************** Installing Layer 4.5 local ***************"

$DCA_KERNEL_DIR/bash/installer.sh

for module in end_dns; do

    echo "*************** Installing $module on client ***************"

    cd $GENI_MOD_DIR
    make module=${module}_client.o
    insmod ${module}_client.ko destination_ip=$SERVER_IP
    cd $GENI_SCRIPT_DIR

    sleep 2

    echo "$module Loaded" >>$OUTPUT

    echo "*************** starting cust batch ***************"

    for ((i = 1; i <= $1; i++)); do
        echo "DNS test $i"
        total=0
        for ((j = 1; j <= 5; j++)); do
            query="www.stanford.edu"

            dig +time=5 +tries=2 @$SERVER_IP -p 53 $query | grep -A 2 "ANSWER SECTION" >>$OUTPUT

            sleep 2
        done

    done

    echo "*************** finished $module cust test ***************"

    sleep 2

    echo "cleaning up $module"

    rmmod ${module}_client

done
