import argparse
import random
import ipaddress  # Import for subnet matching

from CIB_helper import *
import cfg

import logging

logger = logging.getLogger(__name__)

logger.info("CPCON Helper started")

parser = argparse.ArgumentParser(description="Module deploy program")
parser.add_argument(
    "--module", type=str, required=False, help="Module you want to deploy"
)
parser.add_argument("--host", type=int, required=False, help="Host to deploy module to")
parser.add_argument(
    "--inverse", type=str, required=False, help="Module middlebox dependency"
)
parser.add_argument(
    "--type", type=str, required=False, help="type of middlebox for dependency"
)
parser.add_argument("--activate", help="Activates module", action="store_true")
parser.add_argument(
    "--priority",
    type=int,
    required=False,
    help="Module priority level, defaults to random value",
)
parser.add_argument(
    "--applyNow",
    help="Apply module check to all sockets, including previously checked",
    action="store_true",
)

args = parser.parse_args()

active_mode = cfg.active_mode  # default value
applyNow = cfg.applyNow  # default value

db_connection = db_connect(cfg.nco_dir + "cib.db")

if args.activate:
    active_mode = 1  # overwrite default

if args.applyNow:
    applyNow = 1  # overwrite default

if args.priority:
    priority = args.priority
else:
    priority = random.randint(1, 65534)

# Select all the hosts in database
hosts = select_all_hosts(db_connection)
print("Hosts in database:")
for host in hosts:
    print(host)

# Define DMZ subnet, to protect servers
target_subnet = ipaddress.IPv4Network("10.0.8.0/24")

# List to store host_ids of hosts in the target subnet
hosts_in_subnet = []

# Build Kill Module for each host and check for subnet match
for host in hosts:
    host_id = host[1]  # Extract host_id
    host_ip = host[2]  # Extract host_ip

    # Insert the "MILCOM_isolate" module for each host
    insert_req_build_module(db_connection, host_id, "MILCOM_isolate", 1, 1, 0, 1)

    # Check if the host IP is in the target subnet
    if ipaddress.IPv4Address(host_ip) in target_subnet:
        hosts_in_subnet.append(host_id)

# Print the host_ids of hosts in the target subnet
print("Host IDs in the 10.0.8.0/24 subnet:", hosts_in_subnet)

# Build and deploy threat specific module
# For this experiment, module only deploys to servers
for host_id in hosts_in_subnet:
    insert_req_build_module(
        db_connection, host_id, "MILCOM_server", 1, priority, applyNow, 0
    )
