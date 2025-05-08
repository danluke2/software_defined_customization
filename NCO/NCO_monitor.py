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
        for module in alert_modules:
            ids_value = module.get("IDS", "")
            if ids_value.startswith("ALERT:"):
                ids_value = ids_value[6:]  # Extract everything after "ALERT:"
            if ids_value == "ALERT:DNS_DoS":
                event_type = "DNS"
            elif ids_value == "ALERT:STRING":
                event_type = "STRING"
            elif ids_value == "ALERT:STRING1":
                event_type = "STRING1"
            elif ids_value == "ALERT:CPCON3":
                event_type = "CPCON3"
            else:
                logger.info(f"Unknown alert type: {ids_value}")

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

        # correlate event type to response module
        # TO DO, find a better way to do this
        if event_type == "DNS_DoS":
            response_module = "DNS_response"
        elif event_type == "IDS_web_logger":
            response_module = "IDS_web_logger"
        else:
            logger.info(f"Unknown event type: {event_type}")
            return

        # check if response module is already built
        response_built = select_built_module(db_connection, host_id, response_module)

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

        # If module is not in built table or deployed, then insert into req_build

        # send ACK to host for each alert
        acknowledge_alert(conn_socket, host_id, cust_id)

        # update host interval to 5 seconds
        mac = get_mac_by_host_id(db_connection, host_id)
        if mac:
            # Update interval to 5 seconds
            update_host(db_connection, mac, "interval", 5)
            logger.info(f"Updated interval for host {host_id} to 5")
        else:
            logger.info(
                f"Host ID {host_id} not found in database, skipping interval update."
            )

        # insert module into req_build table for ALERTING host
        insert_req_build_module(db_connection, host_id, response_module, 1, 40, 1, 0)
        logger.info(
            f"Module {response_module} inserted into req_build table for host {host_id}, ApplyNow"
        )

    # # Find alert modules
    # alert_modules = [
    #     module for module in deployed_list if module.get("IDS", "").startswith("ALERT")
    # ]

    # # # Determine alert type
    # alert_type = None
    # for module in alert_modules:
    #     ids_value = module.get("IDS", "")
    #     if ids_value == "ALERT:DNS":
    #         alert_type = "DNS"
    #     elif ids_value == "ALERT:STRING":
    #         alert_type = "STRING"
    #     elif ids_value == "ALERT:STRING1":
    #         alert_type = "STRING1"
    #     elif ids_value == "ALERT:CPCON3":
    #         alert_type = "CPCON3"
    #     else:
    #         logger.info(f"Unknown alert type: {ids_value}")

    # # Process alert if it's new for this host
    # if alert_modules and host_id not in processed_alerts:
    #     processed_alerts.add(host_id)

    #     for module in alert_modules:
    #         cust_id = module.get("ID")
    #         logger.info(f"Processing ALERT for host: {host_id} cust_id: {cust_id}")

    #         # Send acknowledgment for each ALERT
    #         acknowledge_alert(conn_socket, host_id, cust_id)

    #         mac = get_mac_by_host_id(db_connection, host_id)
    #         if mac:
    #             # Update interval to 5 seconds
    #             update_host(db_connection, mac, "interval", 5)
    #             logger.info(f"Updated interval for host {host_id} to 5")
    #         else:
    #             logger.info(
    #                 f"Host ID {host_id} not found in database, skipping interval update."
    #             )

    #     # Determine module name based on alert type
    #     # TO DO: Update logic to handle alert string(s)
    #     if alert_type == "DNS":
    #         # Check if module is already built
    #         response_module = select_built_module(
    #             db_connection, host_id, "DNS_response"
    #         )
    #         if response_module:
    #             logger.info(f"Response module: {response_module} already built")
    #             # extract mod_id from response_module
    #             mod_id = response_module[2]
    #             # Module already built, set require_install to 1
    #             ts = int(
    #                 time.time()
    #             )  # TO DO add function to retrieve ts from built_module table
    #             update_built_module_install_requirement(
    #                 db_connection, host_id, mod_id, 1, ts
    #             )
    #             logger.info(f"Set require_install to 1 for module with mod_id 1")
    #             # Skip insert_req_build_module call
    #             module_name = None  # Ensure module_name is not set
    #         else:
    #             # Check if the host id has already been sent the mod_id
    #             deployed_modules = select_deployed_modules(db_connection, host_id)
    #             if any(module["mod_id"] == 1 for module in deployed_modules):
    #                 logger.info(f"Module with mod_id 1 already sent to host {host_id}")
    #                 module_name = None  # Ensure module_name is not set
    #             else:
    #                 # if module is not in built table or deployed, then insert into req_build
    #                 module_name = "DNS_response"

    #     elif alert_type == "STRING":
    #         module_name = "IDS_web_logger"

    #     elif alert_type == "STRING1":
    #         get_log_data(conn_socket, db_connection, host_id)
    #         module_name = None  # Ensure module_name is not set

    #     elif alert_type == "CPCON3":
    #         # Check if module is already built
    #         response_module = select_built_module(
    #             db_connection, host_id, "MILCOM_isolate"
    #         )
    #         if response_module:
    #             logger.info(f"Response module: {response_module} already built")
    #             # extract mod_id from response_module
    #             mod_id = response_module[2]
    #             # Module already built, set require_install to 1
    #             ts = int(
    #                 time.time()
    #             )  # TO DO add function to retrieve ts from built_module table
    #             update_built_module_install_requirement(
    #                 db_connection, host_id, mod_id, 1, ts
    #             )
    #             logger.info(f"Set require_install to 1 for module with mod_id 1")
    #             # Skip insert_req_build_module call
    #             module_name = None  # Ensure module_name is not set

    #     logger.info(f"host_id: {host_id}, module name: {module_name}")

    #     # Check if module needs to be built
    #     # Only call insert_req_build_module if module_name is set
    #     if module_name is not None:
    #         # insert module into req_build table for ALERTING host
    #         insert_req_build_module(db_connection, host_id, module_name, 1, 40, 1, 0)
    #         logger.info(
    #             f"Module {module_name} inserted into req_build table for host {host_id}, ApplyNow"
    #         )

    #         # get all host ids in the database [2] = host_id
    #         all_host_ids = [row[1] for row in select_all_hosts(db_connection)]
    #         logger.info(f"All host ids in database: {all_host_ids}")

    #         # remove hosts in processed_alerts from all_host_ids
    #         all_host_ids = [
    #             host_id for host_id in all_host_ids if host_id not in processed_alerts
    #         ]
    #         logger.info(f"All host ids not alerting: {all_host_ids}")

    #         # build module for hosts NOT alerting
    #         for host_id in all_host_ids:
    #             insert_req_build_module(
    #                 db_connection, host_id, module_name, 1, 41, 1, 0
    #             )
    #             logger.info(
    #                 f"Module {module_name} inserted into req_build table for host {host_id}, no ALERT"
    #             )

    # # Reset interval when ALERT clears
    # elif not alert_modules and host_id in processed_alerts:
    #     processed_alerts.remove(host_id)
    #     mac = get_mac_by_host_id(db_connection, host_id)
    #     if mac:
    #         # Reset interval to default value (30 seconds)
    #         update_host(db_connection, mac, "interval", 30)
    #         logger.info(f"ALERT cleared for host {host_id}, reset interval to {30}")
    #     else:
    #         logger.info(f"Host ID {host_id} not found in database, skipping reset.")

    # return 0


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
