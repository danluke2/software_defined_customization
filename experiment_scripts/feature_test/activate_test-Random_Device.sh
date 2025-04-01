#!/bin/bash

# Purpose: deploy non-activated module and ensure it attaches to socket but not customize it
# Then we send an activate command to ensure cust happens

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

# Force root
if [[ "$(id -u)" != "0" ]]; then
    echo "This script must be run as root" 1>&2
    exit -1
fi

# ----------------------------------------------------------------
# Function for dns test
#   Accepts 1 argument:
#     test name
# ----------------------------------------------------------------
conduct_dns() {
    for ((j = 1; j <= 2; j++)); do
        query="www.test_$1_$j.com"
        dig @$SERVER_IP -p 53 $query >/dev/null
        sleep 0.5
    done
}

echo "Installing Layer 4.5 on server and client"

sshpass -p "$SERVER_PASSWD" ssh -p 22 -o StrictHostKeyChecking=no root@$SERVER_IP "pkill python3; rmmod layer4_5; systemctl stop dnsmasq.service; systemctl stop systemd-resolved.service; $DCA_KERNEL_DIR/bash/installer.sh; dnsmasq --no-daemon -c 0 >/dev/null 2>&1 &"

sleep 1

rmmod layer4_5

$DCA_KERNEL_DIR/bash/installer.sh

sleep 2

# start NCO process with command line params
echo "*************** Starting NCO on Client  ***************"
gnome-terminal -- bash -c "echo '*************** Starting NCO  ***************'; python3 $NCO_DIR/NCO.py --query_interval 5 --build_interval 3 --linear --print --ip $CLIENT_IP --logfile $NCO_DIR/activated_nco_messages.log"

sleep 3

# start client DCA process, which will have host_id = 1
echo "*************** Starting DCA on Client  ***************"
gnome-terminal -- bash -c "echo '*************** Client DCA  ***************'; python3 $DCA_USER_DIR/DCA.py --print --logfile $DCA_USER_DIR/activated_client_dca_messages.log --ip $CLIENT_IP"

sleep 5

# Insert client module in DB for deployment to host_id = 1, with active mode set to false (default)
echo "*************** Deploy Client Module  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "demo_dns_client_app_tag" --host 1 --priority 42

sleep 5

# start DCA process on server, which will have host_id = 2
echo "*************** Starting DCA on Server  ***************"
sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "python3 $DCA_USER_DIR/DCA.py --ip $CLIENT_IP --logfile $DCA_USER_DIR/activated_server_dca_messages.log  >/dev/null 2>&1 &"

sleep 5

# Insert server module in DB for deployment to host_id = 2, with active mode set to false and with applyNow set
echo "*************** Deploy Server Module  ***************"
python3 $NCO_DIR/deploy_module_helper.py --module "demo_dns_server_app_tag" --host 2 --priority 42 --applyNow

sleep 5

# start middlebox collection process
echo "*************** Starting Middlebox DCA on Client  ***************"
gnome-terminal -- bash -c "echo '*************** Starting TCPDUMP  ***************'; tcpdump port 53 -i any -w $GIT_DIR/active.pcap"

sleep 10

echo "*************** starting applyNow, non-activated test ***************"

conduct_dns "apply_not_activated"

echo "*************** finished applyNow and non-activated test ***************"

# now we restart dnsmasq, which should still not get customized since in non-activated status
echo "*************** Restarting DNSMASQ on Server  ***************"
sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill dnsmasq; dnsmasq --no-daemon -c 0 >/dev/null 2>&1 &"

sleep 2

echo "*************** starting non-activated only test ***************"

conduct_dns "not-activated"

echo "*************** finished non-activated only test  ***************"

# Now we enable the server and client modules and make sure they are applied
echo "*************** Toggle Server Module On ***************"
python3 $NCO_DIR/toggle_module_helper.py --module "demo_dns_server_app_tag" --host 2 --activate

sleep 2

echo "*************** Toggle Client Module On ***************"
python3 $NCO_DIR/toggle_module_helper.py --module "demo_dns_client_app_tag" --host 1 --activate

sleep 15 # give plenty of time for module to reflect new active status

echo "*************** starting cust enabled test ***************"

conduct_dns "activated"

echo "*************** finished cust enabled test ***************"

sleep 2

# Revoke server module in DB host_id = 2
echo "*************** Revoke Server Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "demo_dns_server_app_tag" --host 2

sleep 2

# Revoke client module in DB host_id = 1
echo "*************** Revoke Client Module ***************"
python3 $NCO_DIR/revoke_module_helper.py --module "demo_dns_client_app_tag" --host 1

sleep 15

echo "cleaning up"

# save tracelog files from each host
sudo cp /sys/kernel/tracing/trace $EXP_SCRIPT_DIR/logs/activated_client.txt

rmmod layer4_5
pkill tcpdump

sshpass -p "$SERVER_PASSWD" ssh -p 22 root@$SERVER_IP "pkill python3; pkill dnsmasq; systemctl start systemd-resolved.service; sudo cp /sys/kernel/tracing/trace $EXP_SCRIPT_DIR/logs/activated_server.txt; rmmod layer4_5"

pkill python3

# TODO: run a python file to determine success/failure of test automatically
# parse log files and pcap
# activated*.txt: identify activate NLMSG and subsequent L4.5 call, insmod with app name
# parse pcap: no tag in first 10 dns request, and tag in last 5
