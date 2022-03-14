#!/bin/bash

#Purpose: perform the NCO/DCA module deployment experiment
# $1 = number of trials
# $2 = number of hosts


GIT_DIR=/home/vagrant/software_defined_customization
EXP_DIR=$GIT_DIR/experiment_scripts

# NOTE: reverse this order to speed up tests
# NOTE2: if reversing, can also remove --construct flag after the 250 test is
# completed to avoid re-making each module
for hosts in 10 50 100 175 250
do
  echo "*************** Performing Experiment with $hosts Hosts ***************"
  $EXP_DIR/nco_dca_experiment.sh $1 $hosts $GIT_DIR $EXP_DIR
done


echo generating plot

python3 nco_dca_plot.py
