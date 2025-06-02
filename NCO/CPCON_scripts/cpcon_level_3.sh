#!/bin/bash
# Script for CPCON 3 enforcement + verification

# ************** STANDARD PARAMS MUST GO HERE ****************
CPCON_DIR="/home/vagrant/software_defined_customization/NCO/CPCON_scripts"
NCO_DIR="/home/vagrant/software_defined_customization/NCO"
KEY_PATH="/home/vagrant/.ssh/id_rsa"
TARGET_NODE="10.0.2.20"
SUBNET="10.0.2.0/24"

# Step 1: Build kill module in new terminal
gnome-terminal -- bash -c "
    echo '*************** Building Kill Module for all Hosts ***************';
    sudo python3 $NCO_DIR/cpcon_helper.py;
    sleep 10
"

# Step 2: Block HTTP/HTTPS, verify with Nmap, and update DB if successful
gnome-terminal -- bash -c "
    echo '*** Blocking non-essential HTTP/HTTPS ***';
    ansible-playbook -i '${TARGET_NODE},' --private-key='${KEY_PATH}' '${CPCON_DIR}/WEB_Block.yaml';
    if [ \$? -ne 0 ]; then
        echo 'Ansible playbook failed. Exiting.';
        sleep 10;
        exit 1;
    fi

    echo '*** Running nmap scan on ports 80 and 443 ***';
    NMAP_RESULT=\$(nmap -sT -Pn -p 80,443 ${SUBNET});
    echo \"\$NMAP_RESULT\";

    OPEN_PORTS=\$(echo \"\$NMAP_RESULT\" | grep -E '80/tcp[[:space:]]+open|443/tcp[[:space:]]+open' | wc -l);

    if [ \"\$OPEN_PORTS\" -eq 0 ]; then
        echo '*** All ports 80 and 443 are blocked. Verifying policy in DB. ***';
        python3 -c \"import sys; sys.path.insert(0, '${NCO_DIR}'); import sqlite3 as sl; from CIB_helper import mark_policy_verified; con = sl.connect('${NCO_DIR}/cib.db'); mark_policy_verified(con, '3', 'Denial of Service', None)\";
    else
        echo '*** One or more hosts still have ports open. Skipping DB update. ***';
    fi
    echo '*** This window will close in 10 seconds ***';
    sleep 10;
"


