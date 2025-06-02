#!/bin/bash
# Script for CPCON 2

CPCON_DIR="/home/vagrant/software_defined_customization/NCO/CPCON_scripts"
NCO_DIR="/home/vagrant/software_defined_customization/NCO"
KEY_PATH="/home/vagrant/.ssh/id_rsa"
TARGET_NODE="10.0.2.20"
SUBNET="10.0.2.0/27"  # Reduced subnet size for testing

echo "*************** Isolating Non-essential Subnet ***************"
ansible-playbook -i "${TARGET_NODE}," --private-key="$KEY_PATH" "$CPCON_DIR/SUBNET_Block.yaml"

if [ $? -ne 0 ]; then
    echo "Ansible playbook failed. Exiting."
    exit 1
fi

echo "*** Running Nmap TCP port scan ***"
NMAP_RESULT=$(nmap -sT -Pn --top-ports 10 -T4 "$SUBNET" -vv)
echo "$NMAP_RESULT"

echo "*** Running Nmap scan on key UDP ports (53, 67, 68, 123, 161) ***"
UDP_RESULT=$(sudo nmap -sU -p 53,67,68,123,161 -T4 "$SUBNET")
echo "$UDP_RESULT"

VIOLATING_HOSTS=$(echo "$NMAP_RESULT" | awk '
    /Nmap scan report for/ {ip=$NF}
    /^[0-9]+\/tcp[[:space:]]+open/ {
        if (!($1 == "22/tcp" && ip == "10.0.2.20")) {
            print ip
        }
    }' | sort -u)


UDP_VIOLATORS=$(echo "$UDP_RESULT" | awk '
    /Nmap scan report for/ {ip=$NF}
    /^[0-9]+\/udp[[:space:]]+open/ { print ip }
' | sort -u)

if [ -z "$VIOLATING_HOSTS" ] && [ -z "$UDP_VIOLATORS" ]; then
    echo "*** Subnet is correctly isolated. Verifying policy. ***"
    python3 -c "import sys; sys.path.insert(0, '$NCO_DIR'); import sqlite3 as sl; from CIB_helper import mark_policy_verified; con = sl.connect('$NCO_DIR/cib.db'); mark_policy_verified(con, '2', 'Web attacks', None)"
    # python3 -c "import sys; sys.path.insert(0, '$NCO_DIR'); import sqlite3 as sl; from CIB_helper import mark_policy_verified; con = sl.connect('$NCO_DIR/cib.db'); mark_policy_verified(con, '2', 'Web attacks', None)"
else
    echo "*** Isolation FAILED. Violations found on:"
    [ -n "$VIOLATING_HOSTS" ] && echo "  TCP: $VIOLATING_HOSTS"
    [ -n "$UDP_VIOLATORS" ] && echo "  UDP: $UDP_VIOLATORS"
fi

