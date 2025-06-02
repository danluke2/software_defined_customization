#!/usr/bin/env python3

import socket
import os
import json
import random
import shutil
import time  # for copy files

import cfg
from CIB_helper import *
from NCO_revoke import handle_revoke_update
from NCO_deploy import handle_deployed_update

import logging
import re

logger = logging.getLogger(__name__)  # use module name


######################### MONITORING FUNCTIONS #############################


def request_report(conn_socket, host_id):
    command = {"cmd": "run_report"}
    send_string = json.dumps(command, indent=4)
    conn_socket.sendall(bytes(send_string, encoding="utf-8"))
    logger.info(f"Sent report request to {host_id}")


processed_alerts = set()


def process_report(conn_socket, db_connection, host_id, buffer_size):

    global processed_alerts

    logger.info(f"Processing report for host: {host_id}")

    try:
        data = conn_socket.recv(buffer_size)
        json_data = json.loads(data)
    except json.decoder.JSONDecodeError as e:
        logger.info(f"Error on process report recv call, {e}")
        return cfg.CLOSE_SOCK
    # TODO: check report mac matches host_id and update DB?
    deployed_list = json_data["Installed"]
    deprecated_list = json_data["Deprecated"]
    revoked_list = json_data["Revoked"]
    alert_list = json_data["Alerted"]

    # update deployed table based on revoked list
    handle_revoke_update(db_connection, host_id, revoked_list)
    # update deployed table based on active list
    handle_deployed_update(db_connection, host_id, deployed_list, deprecated_list)

    # Decode received data for processing
    data_str = data.decode("utf-8")
    logger.info(f"Process Report String: {data_str}")

    # Look for alerting modules in the deployed list
    alert_modules = [
        {
            "ID": module.get("ID"),
            "event_type": module.get("IDS", "")[
                6:
            ],  # Extract everything after "ALERT:"
        }
        for module in deployed_list
        if module.get("IDS", "").startswith("ALERT:")
    ]
    # TO DO: get rid of this declaration
    event_type = None

    # Parse the alert modules to get event_type and cust_id
    if alert_modules:
        # verify if host is already alerting
        all_host_alerts = select_all_alerts(db_connection)

        for module in alert_modules:
            event_type = module["event_type"]
            cust_id = module["ID"]
            logger.info(
                f"Processing ALERT for host: {host_id}, cust_id: {cust_id}, event_type: {event_type}"
            )
            # check if event_type is already associated with host_id, if not then insert
            if host_id in all_host_alerts and event_type in all_host_alerts[host_id]:
                logger.info(
                    f"Event type {event_type} already reported by host {host_id}"
                )
                return
            else:
                insert_alert(db_connection, host_id, event_type)
                logger.info(
                    f"Inserted alert for event type {event_type} from host {host_id}"
                )

        # TO DO: Find a better way to do this
        # Find the eventy_types in alert_modules and assign a response

        for module in alert_modules:
            if event_type == "DNS_DoS":
                response_module = "DNS_response"
            elif event_type == "DNS_DoS_response":
                response_module = None
            elif event_type == "STRING1":
                response_module = "STRING1"
            elif event_type == "CPCON3":
                response_module = "CPCON3"
            else:
                logger.info(f"Unknown event type: {event_type}")
                response_module = None
                return

        # Send acknowledgment for each ALERT module
        acknowledge_alert(conn_socket, host_id, cust_id)

        mac = get_mac_by_host_id(db_connection, host_id)
        if mac:
            # Update interval to 5 seconds
            update_host(db_connection, mac, "interval", 5)
            logger.info(f"Updated interval for host {host_id} to 5")
        else:
            logger.info(
                f"Host ID {host_id} not found in database, skipping interval update."
            )

        if response_module is not None:
            # if CPCON alert, send kill module to host
            if response_module == "CPCON3":
                update_built_module_install_requirement(
                    db_connection, host_id, "MILCOM_isolate", 1, 0
                )

            # check if response module is already built
            response_built = select_built_module(
                db_connection, host_id, response_module
            )

            if response_built:
                logger.info(f"Response module: {response_built} already built")
                # deploy module to host
                mod_id = response_built[2]
                ts = int(
                    time.time()
                )  # TO DO add function to retrieve ts from built_module table
                update_built_module_install_requirement(
                    db_connection, host_id, mod_id, 1, ts
                )
            else:
                # Check if the host id has already been sent the mod_id
                deployed_modules = select_deployed_modules(db_connection, host_id)
                if any(module["mod_id"] == mod_id for module in deployed_modules):
                    logger.info(
                        f"Module with mod_id {mod_id} already sent to host {host_id}"
                    )

            # insert module into req_build table for ALERTING host
            insert_req_build_module(
                db_connection, host_id, response_module, 1, 40, 1, 0
            )
            logger.info(
                f"Module {response_module} inserted into req_build table for host {host_id}, ApplyNow"
            )

    # TO DO: fix this logic
    # Reset interval if alerting module has been logged
    elif select_specific_host_alert(db_connection, host_id, event_type) is not None:
        mac = get_mac_by_host_id(db_connection, host_id)

        if mac:
            # Reset interval to default value (30 seconds)
            update_host(db_connection, mac, "interval", 30)
            logger.info(f"ALERT cleared for host {host_id}, reset interval to 30")
        else:
            logger.info(f"Host ID {host_id} not found in database, skipping reset.")

    return 0


def get_log_data(conn_socket, db_connect, host_id):
    """Request log data from host"""
    command = {"cmd": "get_log_data"}
    logger.info(f"Sending get log data request to {host_id}, command: {command}")
    send_string = json.dumps(command, indent=4)
    conn_socket.sendall(bytes(send_string, encoding="utf-8"))

    """Receive log data from host"""
    try:
        data = conn_socket.recv(4096)
        url_data = json.loads(data)
    except json.decoder.JSONDecodeError as e:
        logger.info(f"Error on process report recv call, {e}")
        return cfg.CLOSE_SOCK

    logger.info(f"Malicious URL data: {url_data}")

    # extract the URL from the data
    blocked_hosts = url_data.get("blocked_hosts", [])
    if blocked_hosts:
        logger.info(f"Blocked hosts: {blocked_hosts}")
        insert_alert(db_connect, blocked_hosts)

    else:
        logger.info("No blocked hosts found in the log data.")


def get_mac_by_host_id(con, host_id):
    """Retrieve the MAC address for a given host_id."""
    try:
        with con:
            cur = con.execute("SELECT mac FROM hosts WHERE host_id = ?", (host_id,))
            result = cur.fetchone()
            if result:
                return result[0]  # Return the MAC address
    except sl.Error as er:
        logger.info(f"Error retrieving MAC for host_id {host_id}, Error: {er}")
    return None


# IDS function to acknowledge alert detected from DCA
def acknowledge_alert(conn_socket, host_id, cust_id):
    command = {"cmd": f"acknowledge_alert {cust_id}"}
    logger.info(f"Sending acknowledge alert to {host_id}, command: {command}")
    send_string = json.dumps(command, indent=4)
    conn_socket.sendall(bytes(send_string, encoding="utf-8"))
    # logger.info(f"Sent acknowledge alert to {host_id}")


# Build directory structure for storing host files
def setup_host_dirs(host_id):
    # create the host dir based on assigned host_id
    os.mkdir(cfg.nco_dir + cfg.symvers_dir + host_id)
    # create the modules dir based on assigned host_id
    os.mkdir(cfg.nco_dir + cfg.symvers_dir + host_id + "/modules")
    # put generic makefile in modules dir
    newPath = shutil.copy(
        cfg.nco_mod_dir + "Makefile",
        cfg.nco_dir + cfg.symvers_dir + host_id + "/modules",
    )
    # put a copy of common_structs in host dir for all modules
    newPath = shutil.copy(
        cfg.common_struct_dir + "common_structs.h",
        cfg.nco_dir + cfg.symvers_dir + host_id,
    )


def handle_host_insert(db_connection, mac, ip, port, kernel_release, interval):
    max_tries = 10
    counter = 0
    # repeatedly generate host ID and insert into db until successful
    while counter <= max_tries:
        if cfg.random_hosts:
            generated_id = random.randint(1, 65535)
        elif cfg.ip_hosts:
            ip_split = ip.split(".")
            generated_id = int(ip_split[-1])
        else:
            generated_id = cfg.next_module_id
            cfg.next_module_id += 1

        logger.info(f"Inserting host {mac}, ID = {generated_id}")
        # if host_id already exists, then DB error occurs and we try again
        err = insert_host(
            db_connection, mac, generated_id, ip, port, 0, 0, kernel_release, interval
        )
        if err == cfg.DB_ERROR:
            logger.info("Could not insert host, try again")
            if counter == max_tries:
                return cfg.DB_ERROR
            counter += 1
            continue
        else:
            break
    setup_host_dirs(str(generated_id))
    host = select_host(db_connection, mac)
    if host == cfg.DB_ERROR:
        logger.info("Could not retrieve host after insert, DB_ERROR")
    return host
