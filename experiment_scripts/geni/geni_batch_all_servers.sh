#!/bin/bash

#Purpose: perform the NCO/DCA module deployment experiment
# $1 = number of trials
# $2 = number of DNS requests/file request
# $3 = sleep time between each DNS request
# $4 = client IP address for customization modules

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
# ************** END STANDARD PARAMS ****************

# Force root
if [[ "$(id -u)" != "0" ]]; then
  echo "This script must be run as root" 1>&2
  exit -1
fi

# 130.127.215.148 = clemson
# 192.12.245.163 = colorado
# 204.102.228.173 = nps
# 128.206.119.41 = missouri
# 128.171.8.123 = hawaii
# 128.95.190.50 = washington
# 192.122.236.101 = cornell

# 128.112.170.33 = princeton
# 198.82.156.38 = virginia tech
# 72.36.65.68 = illinois
# 165.124.51.195 = northwestern
# 192.86.139.70 = nyu -> works
# 140.254.14.100 = ohio state
# 129.110.253.30 = UT

# first batch
for server in 130.127.215.148 192.12.245.163 204.102.228.173 128.206.119.41 128.171.8.123 128.95.190.50 192.122.236.101; do
  echo "*************** Performing Batch DNS Experiment with Server $server ***************"
  $GENI_SCRIPT_DIR/geni_batch_single_experiment.sh $1 $2 $3 $server $4

  echo "*************** Performing Bulk File Experiment with Server $server ***************"
  $GENI_SCRIPT_DIR/geni_bulk_single_experiment.sh $2 $server $4
done

# second batch
for server in 128.112.170.33 198.82.156.38 72.36.65.68 165.124.51.195 192.86.139.70 140.254.14.100 129.110.253.30; do
  echo "*************** Performing Batch DNS Experiment with Server $server ***************"
  $GENI_SCRIPT_DIR/geni_batch_single_experiment.sh $1 $2 $3 $server $4

  echo "*************** Performing Bulk File Experiment with Server $server ***************"
  $GENI_SCRIPT_DIR/geni_bulk_single_experiment.sh $2 $server $4
done

# 8.8.8.8 8.8.4.4 = Google
# 9.9.9.9 149.112.112.112 = Quad9
# 208.67.222.222 208.67.220.220 = OpenDNS
# 1.1.1.1 1.0.0.1  = Cloudflare
# 185.228.168.9 185.228.169.9 = CleanBrowsing
# 76.76.19.19 76.223.122.150 = Alternate dns
# 94.140.14.14 94.140.15.15 = AdGuard DNS

# public DNS servers: testing End DNS module
for server in 8.8.8.8 8.8.4.4 9.9.9.9 149.112.112.112 208.67.222.222 208.67.220.220 1.1.1.1 1.0.0.1 185.228.168.9 185.228.169.9 76.76.19.19 76.223.122.150 94.140.14.14 94.140.15.15; do
  echo "*************** Performing Batch DNS Experiment with Server $server ***************"
  $GENI_SCRIPT_DIR/geni_public_dns_single_experiment.sh $1 $2 $3 $server $4

done
