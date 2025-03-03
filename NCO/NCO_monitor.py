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
    alert_list = json_data.get("Alerts", [])
    logger.info(f"Alert list: {alert_list}")

    # update deployed table based on revoked list
    handle_revoke_update(db_connection, host_id, revoked_list)
    # update deployed table based on active list
    handle_deployed_update(db_connection, host_id, deployed_list, deprecated_list)

    # Decode received data for processing
    data_str = data.decode("utf-8")
    logger.info(f"Process Report String: {data_str}")
    logger.info(f"Processed alerts: {processed_alerts}")

    # Find alert modules
    alert_modules = [
        module for module in deployed_list if module.get("IDS", "").startswith("ALERT")
    ]

    # Determine alert type
    alert_type = None
    for module in alert_modules:
        ids_value = module.get("IDS", "")
        if ids_value == "ALERT:DNS":
            alert_type = "DNS"
        elif ids_value == "ALERT:STRING":
            alert_type = "STRING"

    # Process alert if it's new for this host
    if alert_modules and host_id not in processed_alerts:
        processed_alerts.add(host_id)
        logger.info(f"ALERT detected for host {host_id}")

        for module in alert_modules:
            cust_id = module.get("ID")
            logger.info(f"Processing ALERT for cust_id: {cust_id}")

            # Send acknowledgment for each ALERT
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

        # Determine module name based on alert type
        if alert_type == "DNS":
            module_name = "DNS_response"
        elif alert_type == "STRING":
            module_name = "IDS_server_MT"

        # Check if module needs to be built
        insert_req_build_module(db_connection, host_id, module_name, 1, 42, 1)

    # Reset interval when ALERT clears
    elif not alert_modules and host_id in processed_alerts:
        processed_alerts.remove(host_id)
        mac = get_mac_by_host_id(db_connection, host_id)
        if mac:
            # Reset interval to default value (30 seconds)
            update_host(db_connection, mac, "interval", 30)
            logger.info(f"ALERT cleared for host {host_id}, reset interval to {30}")
        else:
            logger.info(f"Host ID {host_id} not found in database, skipping reset.")

    return 0


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
    logger.info(f"Sent acknowledge alert to {host_id}")


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
