#!/bin/bash

#Purpose: perform the Layer 4.5 3rd party middlebox interference experiment
# $1 = number of trials
# $2 = client IP address for customization modules

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

# Force root
if [[ "$(id -u)" != "0" ]]; then
  echo "This script must be run as root" 1>&2
  exit -1
fi

# first batch:

# 130.127.215.159 = clemson
# 192.12.245.165 = colorado
# 128.206.119.40 = missouri
# 128.171.8.123 = hawaii
# 128.95.190.54 = washington
# 192.122.236.116 = cornell

# for server in 130.127.215.159 192.12.245.165 128.206.119.40 128.171.8.123 128.95.190.54 192.122.236.116; do
for server in 130.127.215.159 128.171.8.123; do
  mkdir -p $EXP_SCRIPT_DIR/logs/$server
  echo "*************** Performing Batch DNS Experiment with Server $server ***************"
  $GENI_SCRIPT_DIR/middlebox_dns_single.sh $1 $server $2

  # echo "*************** Performing Bulk File Experiment with Server $server ***************"
  # $GENI_SCRIPT_DIR/middlebox_web_single.sh $1 $server $2
done

# second batch:

# 198.82.156.49 = virginia tech
# 72.36.65.88 = illinois
# 165.124.51.192 = northwestern
# 192.86.139.73 = nyu -> works
# 140.254.14.101 = ohio state
# 129.110.253.31 = UT

# for server in 198.82.156.49 72.36.65.88 165.124.51.192 192.86.139.73 140.254.14.101 129.110.253.31; do
#   mkdir -p $EXP_SCRIPT_DIR/logs/$SERVER_IP
#   echo "*************** Performing Batch DNS Experiment with Server $server ***************"
#   $GENI_SCRIPT_DIR/middlebox_dns_single.sh $1 $server $2

#   echo "*************** Performing Bulk File Experiment with Server $server ***************"
#   $GENI_SCRIPT_DIR/middlebox_web_single.sh $1 $server $2
# done

# public DNS servers: testing End DNS module

# 8.8.8.8 8.8.4.4 = Google
# 9.9.9.9 149.112.112.112 = Quad9
# 208.67.222.222 208.67.220.220 = OpenDNS
# 1.1.1.1 1.0.0.1  = Cloudflare
# 185.228.168.9 185.228.169.9 = CleanBrowsing
# 76.76.19.19 76.223.122.150 = Alternate dns
# 94.140.14.14 94.140.15.15 = AdGuard DNS

# for server in 8.8.8.8 8.8.4.4 9.9.9.9 149.112.112.112 208.67.222.222 208.67.220.220 1.1.1.1 1.0.0.1 185.228.168.9 185.228.169.9 76.76.19.19 76.223.122.150 94.140.14.14 94.140.15.15; do
# mkdir -p $EXP_SCRIPT_DIR/logs/$SERVER_IP
#   echo "*************** Performing Batch DNS Experiment with Server $server ***************"
#   $GENI_SCRIPT_DIR/middlebox_dns_public.sh $1 $server $2

# done
