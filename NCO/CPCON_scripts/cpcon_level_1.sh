#!/bin/bash
# script for CPCON 1
echo "Setting CPCON 1"
gnome-terminal -- bash -c "echo '*************** Isolating Non-essential Subnet  ***************'; ansible-playbook -i $target_node, SUBNET_Block.yml"
