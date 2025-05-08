#!/bin/bash
# script for CPCON 3

# ************** STANDARD PARAMS MUST GO HERE ****************
CPCON_DIR=/home/vagrant/software_defined_customization/NCO/CPCON_scripts
NCO_DIR=/home/vagrant/software_defined_customization/NCO
target_node="10.0.2.20"

# build kill module for all hosts connected to CPCON orchestrator
gnome-terminal -- bash -c "echo '*************** Building Kill Module for all Hosts  ***************'; sudo python3 $NCO_DIR/cpcon_helper.py; sleep 30"

# deploy ansible playbook to block HTTP/HTTPS traffic from non-essential nodes
gnome-terminal -- bash -c "echo '*** Blocking non-essential HTTP/HTTPS ***'; ansible-playbook -i "$target_node", --private-key=/home/vagrant/.ssh/id_rsa $CPCON_DIR/WEB_Block.yaml -u vagrant; sleep 30"



