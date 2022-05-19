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

USERNAME=
PASSWORD=


# ************** END STANDARD PARAMS  ****************

SERVER_IP=$2
CLIENT_IP=$3


# Force root
if [[ "$(id -u)" != "0" ]];
then
	echo "This script must be run as root" 1>&2
	exit -1
fi


#check if overhead file is present before starting
FILE=$GENI_SCRIPT_DIR/layer4_5.ko
if [ -f "$FILE" ];
then
    echo "$FILE check passed"
else
	echo "Could not find required overhead file" 1>&2
	exit -1
fi

echo "Calculating file checksum"
MD5=($(md5sum $FILE))



# create file to store md5 sums
OUTPUT=$EXP_SCRIPT_DIR/logs/bulk_${2}.txt
touch $OUTPUT

echo "$SERVER_IP" >> $OUTPUT
echo "Baseline" >> $OUTPUT

sshpass -P passphrase -p "$PASSWORD" ssh -t -p 22 -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$SERVER_IP "sudo rmmod layer4_5; sudo pkill python3; sudo systemctl restart simple_server.service"


sleep 2

rmmod layer4_5

# store md5 sum at start of file for comparison
echo $MD5 >> $OUTPUT

echo "*************** starting baseline downloads ***************"

# download file to VM desktop to avoid using shared disk space
cd $GIT_DIR/../Desktop

for ((i=1;i<=$1;i++))
do
  echo "Download $i";
  curl http://$SERVER_IP:8080/users/${USERNAME}/software_defined_customization/experiment_scripts/geni/layer4_5.ko -o test.ko;
  sum=($(md5sum test.ko));
  echo "$sum" >> $OUTPUT;
  rm test.ko;
  sleep 2;
done


echo "*************** finished baseline test ***************"




echo "*************** Installing Layer 4.5 on server ***************"

cd $GENI_SCRIPT_DIR

sshpass -P passphrase -p "$PASSWORD" ssh -t -p 22 -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$SERVER_IP "sudo pkill python3; sudo ~/software_defined_customization/DCA_kernel/bash/installer.sh"

sleep 2

echo "*************** Installing Layer 4.5 local ***************"


$DCA_KERNEL_DIR/bash/installer.sh;



for module in bulk_1k bulk_100 bulk_50
do

	echo "*************** Installing $module on server ***************"

	sshpass -P passphrase -p "$PASSWORD" ssh -t -p 22 -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$SERVER_IP "cd ~/software_defined_customization/layer4_5_modules/geni; make BUILD_MODULE=${module}_server.o; sudo insmod ${module}_server.ko destination_ip=$CLIENT_IP source_ip=$SERVER_IP; sudo systemctl restart simple_server.service"


	sleep 2

	echo "*************** Installing $module on client ***************"


	cd $GENI_MOD_DIR
	make BUILD_MODULE=${module}_client.o
	insmod ${module}_client.ko destination_ip=$SERVER_IP
	cd $GENI_SCRIPT_DIR

	sleep 2

	echo "$module Loaded" >> $OUTPUT


	echo "*************** starting $module cust test ***************"

	# download file to VM desktop to avoid using shared disk space
	cd $GIT_DIR/../Desktop

	for ((i=1;i<=$1;i++))
	do
	  echo "Download $i";
	  curl http://$SERVER_IP:8080/users/${USERNAME}/software_defined_customization/experiment_scripts/geni/layer4_5.ko -o test.ko;
	  sum=($(md5sum test.ko));
	  echo "$sum" >> $OUTPUT;
	  rm test.ko;
	  sleep 2;
	done

	echo "*************** finished $module cust test ***************"

	sleep 2

	echo "cleaning up $module"

	rmmod ${module}_client


	cd $GENI_SCRIPT_DIR
	sshpass -P passphrase -p "$PASSWORD" ssh -t -p 22 -i id_geni_ssh_rsa -o StrictHostKeyChecking=no $USERNAME@$SERVER_IP "sudo pkill python3; sudo rmmod ${module}_server; exit"

done
