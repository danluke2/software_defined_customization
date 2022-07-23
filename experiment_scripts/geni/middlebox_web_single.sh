#!/bin/bash

#Purpose: perform downloads of test file for each config
# $1 = number of downloads (number of trials to perform)
# $2 = server to connect to
# $3 = client IP address for customization modules

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
USERNAME=dflukasz
PASSWORD=geni_ssh
# ************** END STANDARD PARAMS  ****************

SERVER_IP=$2
CLIENT_IP=$3

# Force root
if [[ "$(id -u)" != "0" ]]; then
	echo "This script must be run as root" 1>&2
	exit -1
fi

#check if overhead file is present before starting
FILE=$GENI_SCRIPT_DIR/layer4_5.ko
if [ -f "$FILE" ]; then
	echo "$FILE check passed"
else
	echo "Could not find required overhead file" 1>&2
	exit -1
fi

echo "Calculating file checksum"
MD5=($(md5sum $FILE))

# create file to store md5 sums
OUTPUT=$EXP_SCRIPT_DIR/logs/$SERVER_IP/middlebox_web_${2}.txt
touch $OUTPUT

echo "$SERVER_IP" >>$OUTPUT
echo "Baseline" >>$OUTPUT

sshpass -P passphrase -p "$PASSWORD" ssh -t -p 22 -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$SERVER_IP "sudo rmmod layer4_5; sudo pkill python3; sudo systemctl restart simple_server.service"

sleep 2

rmmod layer4_5

# store md5 sum at start of file for comparison
echo $MD5 >>$OUTPUT

echo "*************** starting baseline downloads ***************"

# download file to VM desktop to avoid using shared disk space
cd $GIT_DIR/../Desktop

for ((i = 1; i <= $1; i++)); do
	echo "Download $i"
	curl http://$SERVER_IP:8080/users/${USERNAME}/software_defined_customization/experiment_scripts/geni/layer4_5.ko -o test.ko
	sum=($(md5sum test.ko))
	echo "$sum" >>$OUTPUT
	rm test.ko
	sleep 5
done

echo "*************** finished baseline test ***************"

echo "*************** Installing Layer 4.5 on server ***************"

cd $GENI_SCRIPT_DIR

sshpass -P passphrase -p "$PASSWORD" ssh -t -p 22 -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$SERVER_IP "sudo pkill python3; sudo ~/software_defined_customization/DCA_kernel/bash/installer.sh"

sleep 2

echo "*************** Installing Layer 4.5 local ***************"

$DCA_KERNEL_DIR/bash/installer.sh

for posit in 32000 3200 320 32 16; do

	echo "*************** Installing module on server ($posit) ***************"

	sshpass -P passphrase -p "$PASSWORD" ssh -t -p 22 -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$SERVER_IP "cd ~/software_defined_customization/layer4_5_modules/geni; make BUILD_MODULE=bulk_server.o; sudo insmod bulk_server.ko destination_ip=$CLIENT_IP source_ip=$SERVER_IP BYTE_POSIT=$posit; sudo systemctl restart simple_server.service"

	sleep 2

	echo "*************** Installing module on client ($posit) ***************"

	cd $GENI_MOD_DIR
	make BUILD_MODULE=bulk_client.o
	insmod bulk_client.ko destination_ip=$SERVER_IP BYTE_POSIT=$posit
	cd $GENI_SCRIPT_DIR

	sleep 2

	echo "$posit Module Loaded" >>$OUTPUT

	echo "*************** starting cust test ***************"

	# download file to VM desktop to avoid using shared disk space
	cd $GIT_DIR/../Desktop

	echo "*************** Starting Middlebox DCA on Client  ***************"
	gnome-terminal -- bash -c "echo '*************** Starting TCPDUMP  ***************'; tcpdump tcp port 8080 -i any -w $EXP_SCRIPT_DIR/logs/$SERVER_IP/middle_web_$posit.pcap"

	sleep 5

	for ((i = 1; i <= $1; i++)); do
		echo "Download $i"
		curl http://$SERVER_IP:8080/users/${USERNAME}/software_defined_customization/experiment_scripts/geni/layer4_5.ko -o test.ko
		sum=($(md5sum test.ko))
		echo "$sum" >>$OUTPUT
		rm test.ko
		sleep 5
	done

	echo "*************** finished module cust test ***************"

	sleep 2

	echo "cleaning up module $posit"

	rmmod bulk_client
	pkill tcpdump

	cd $GENI_SCRIPT_DIR
	sshpass -P passphrase -p "$PASSWORD" ssh -t -p 22 -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$SERVER_IP "sudo pkill python3; sudo rmmod bulk_server; exit"

done
