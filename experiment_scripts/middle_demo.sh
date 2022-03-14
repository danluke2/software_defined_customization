#!/bin/bash

#Purpose: perform the middlebox demo
# $1 = standard report interval (ex: 5)

#directory holding the software_defined_customization git repo
GIT_DIR=/home/vagrant/software_defined_customization
SLEEP_INT=5

#install Layer 4.5 on each device
echo "*************** Install L4.5 on Client  ***************"

sshpass -p "vagrant" ssh -p 22 -o StrictHostKeyChecking=no root@10.0.0.40 "rmmod layer4_5; ifconfig enp0s3 down; $GIT_DIR/DCA_kernel/bash/installer.sh $GIT_DIR/DCA_kernel;"

sleep $SLEEP_INT


echo "*************** Install L4.5 on Server  ***************"

#ensure layer4.5 is not currently running and DB doesn't exist
rmmod layer4_5
rm $GIT_DIR/NCO/cib.db
pkill dnsmasq

$GIT_DIR/DCA_kernel/bash/installer.sh $GIT_DIR/DCA_kernel

sleep $SLEEP_INT


# start NCO process with command line params
echo "*************** Starting NCO  ***************"
gnome-terminal -- python3 $GIT_DIR/NCO/NCO.py --query_interval $1 --linear

sleep $SLEEP_INT


# start DCA process, which will have host_id = 1
echo "*************** Starting DCA on Server  ***************"
gnome-terminal --  python3 $GIT_DIR/DCA\_user/DCA.py

sleep $SLEEP_INT


# start DCA middlebox process
echo "*************** Starting Middlebox DCA on Server  ***************"
gnome-terminal --  python3 $GIT_DIR/DCA\_user/DCA_middlebox.py

sleep $SLEEP_INT


# start DCA process on client, which will have host_id = 2
echo "*************** Starting DCA on Client  ***************"
sshpass -p "vagrant" ssh -p 22 root@10.0.0.40 "python3 $GIT_DIR/DCA\_user/DCA.py >/dev/null 2>&1 &"

sleep $SLEEP_INT


# Insert server module in DB for deployment to host_id = 1
echo "*************** Deploy Server Module and Inverse  ***************"
python3 $GIT_DIR/NCO/deploy_module_helper.py --module "demo_dns_server_app_tag" --host 1 --inverse "demo_dns_tag.lua" --type "Wireshark"

sleep $SLEEP_INT
sleep $SLEEP_INT

# Insert client module in DB for deployment to host_id = 2
echo "*************** Deploy Server Module and Inverse  ***************"
python3 $GIT_DIR/NCO/deploy_module_helper.py --module "demo_dns_client_app_tag" --host 2

sleep $SLEEP_INT


# start middlebox collection process
echo "*************** Starting Middlebox DCA on Server  ***************"
gnome-terminal -- tcpdump udp port 53 -i any -w $GIT_DIR/middle_demo.pcap

sleep $SLEEP_INT


# start dnsmasq process on server
echo "*************** Starting DNSMASQ on Server  ***************"
gnome-terminal -- dnsmasq --no-daemon -c 0

sleep $SLEEP_INT


# perform DNS requests from client
echo "*************** Conducting DNS Queries from Client  ***************"
sshpass -p "vagrant" ssh -p 22 root@10.0.0.40 "dig @10.0.0.20 -p 53 www.dig_test.com; curl www.curl_test.com;"

sleep $SLEEP_INT


echo "*************** finished  ***************"


echo "removing dns modules and layer 4.5"
sshpass -p "vagrant" ssh -p 22 root@10.0.0.40 "pkill python; rmmod demo_dns_client_app_tag; rmmod layer4_5; ifconfig enp0s3 up"

sleep $SLEEP_INT

pkill python3
pkill tcpdump
rmmod demo_dns_server_app_tag
rmmod layer4_5


echo "Opening Wireshark PCAP"
wireshark -r $GIT_DIR/middle_demo.pcap
