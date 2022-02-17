#!/bin/bash

#Purpose: perform the NCO/DCA module deployment experiment
# $1 = number of trials
# $2 = number of hosts

#directory holding the software_defined_customization git repo
GIT_DIR=/home/dan/software_defined_customization
EXP_DIR=$GIT_DIR/experiment_scripts
TEMP_DIR=/home/dan/Desktop/temp

# NCO reset DB, start NCO with delay timer for constuction module
# NCO connect to DCA clear temp folder, startup with desired number of hosts ($2)
# NCO mark modules for deployment and repeat experiment for $1 trials


TEMP_FILE=logs/nco_finished.txt

echo "*************** Install L4.5 on DCA  ***************"

sshpass -p "default" ssh -p 22 root@10.0.0.40 "rmmod layer4_5; $GIT_DIR/DCA_kernel/bash/installer.sh;"

sleep 1

echo "*************** starting first trial ***************"

#first test is different than remainder b/c we need to build modules and deploy them
python3 $EXP_DIR/overhead_exp_NCO.py --construct --sleep 5 --number $2 &

sleep 2

sshpass -p "default" ssh -p 22 root@10.0.0.40 "pkill python ; rm -rf $TEMP_DIR ; mkdir $TEMP_DIR; cd $EXP_DIR; python3 overhead_exp_DCA.py --dir $TEMP_DIR --number $2 >/dev/null 2>&1 &"

sleep 10

#wait until NCO informs us it is done with test by writing a temp file
until [ -f $TEMP_FILE ]
do
     sleep 5
done
echo "NCO finished"
rm $TEMP_FILE
sleep 1
pkill python3


echo "*************** finished first trial  ***************"


# loop covers remaining trials for this number of hosts
for ((i=2;i<=$1;i++))
do
  echo "*************** Performing trial $1 ***************"
  python3 $EXP_DIR/overhead_exp_NCO.py  --sleep 5 --number $2 &

  sleep 2

  sshpass -p "default" ssh -p 22 root@10.0.0.40 "pkill python ; rm -rf $TEMP_DIR ; mkdir $TEMP_DIR; cd $EXP_DIR; python3 overhead_exp_DCA.py --dir $TEMP_DIR --number $2 >/dev/null 2>&1 &"

  sleep 10

  #wait until NCO informs us it is done with test
  until [ -f $TEMP_FILE ]
  do
       sleep 5
  done
  echo "NCO finished"
  rm $TEMP_FILE
  sleep 1
  pkill python3
done

echo "*************** finished all trials  ***************"


echo "cleaning up"

pkill python3

sshpass -p "default" ssh -p 22 root@10.0.0.40 "pkill python"
