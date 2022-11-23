#!/bin/bash

#Purpose: perform dns requests to test middlebox interference
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

# create file to store batch times
OUTPUT=$EXP_SCRIPT_DIR/logs/$SERVER_IP/middlebox_dns_${2}.txt
touch $OUTPUT

echo "$SERVER_IP" >>$OUTPUT
echo "Baseline" >>$OUTPUT

sshpass -P passphrase -p "$PASSWORD" ssh -t -p 22 -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$SERVER_IP "sudo rmmod layer4_5; sudo systemctl stop dnsmasq.service; sudo systemctl stop systemd-resolved.service; sudo systemctl restart dnsmasq.service"

sleep 2

rmmod layer4_5

echo "*************** starting baseline batch ***************"

echo "*************** Starting Middlebox DCA on Client  ***************"
gnome-terminal -- bash -c "echo '*************** Starting TCPDUMP  ***************'; tcpdump -i any -w $EXP_SCRIPT_DIR/logs/$SERVER_IP/middle_dns_base.pcap"

echo "starting dns requests to ensure connectivity"

for ((i = 1; i <= $1; i++)); do
    echo "DNS test $i"
    for ((j = 1; j <= 5; j++)); do
        query="www.test_base$i$j.com"
        dig +time=5 +tries=2 @$SERVER_IP -p 53 $query | grep -A 2 "ANSWER SECTION" >>$OUTPUT
        sleep 2
    done
done

echo "*************** finished baseline test ***************"

pkill tcpdump

echo "*************** Installing Layer 4.5 on server ***************"

sshpass -P passphrase -p "$PASSWORD" ssh -t -p 22 -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$SERVER_IP "sudo pkill dnsmasq; sudo ~/software_defined_customization/DCA_kernel/bash/installer.sh"

sleep 2

echo "*************** Installing Layer 4.5 local ***************"

$DCA_KERNEL_DIR/bash/installer.sh

for module in front_dns middle_dns end_dns compress_dns; do

    echo "*************** Installing $module on server ***************"

    sshpass -P passphrase -p "$PASSWORD" ssh -t -p 22 -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$SERVER_IP "cd ~/software_defined_customization/layer4_5_modules/geni; make module=${module}_server.o; sudo insmod ${module}_server.ko destination_ip=$CLIENT_IP source_ip=$SERVER_IP; sudo systemctl restart dnsmasq.service"

    sleep 2

    echo "*************** Installing $module on client ***************"

    cd $GENI_MOD_DIR
    make module=${module}_client.o
    insmod ${module}_client.ko destination_ip=$SERVER_IP
    cd $GENI_SCRIPT_DIR

    sleep 2

    echo "$module Loaded" >>$OUTPUT

    echo "*************** starting cust batch ***************"

    echo "*************** Starting Middlebox DCA on Client  ***************"
    gnome-terminal -- bash -c "echo '*************** Starting TCPDUMP  ***************'; tcpdump -i any -w $EXP_SCRIPT_DIR/logs/$SERVER_IP/middle_dns_$module.pcap"

    sleep 5

    for ((i = 1; i <= $1; i++)); do
        echo "DNS test $i"
        for ((j = 1; j <= 5; j++)); do
            query="www.test_cust$i$j.com"
            dig +time=5 +tries=2 @$SERVER_IP -p 53 $query | grep -A 2 "ANSWER SECTION" >>$OUTPUT
            sleep 2
        done
    done

    echo "*************** finished $module cust test ***************"

    sleep 2

    echo "cleaning up $module"

    rmmod ${module}_client
    pkill tcpdump

    cd $GENI_SCRIPT_DIR
    sshpass -P passphrase -p "$PASSWORD" ssh -t -p 22 -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$SERVER_IP "sudo pkill dnsmasq; sudo rmmod ${module}_server; exit"

done
