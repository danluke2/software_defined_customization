#!/usr/bin/env python3

import socket
import os
import json
import time

import cfg
from CIB_helper import *

import logging


logger = logging.getLogger(__name__)  # use module name


######################### DEPLOY FUNCTIONS #############################


# Host reported deployed_list, which we compare to the deployed rows in our DB
# We also compare the deployed_list to the install rows and update both tables as necessary
def handle_deployed_update(db_connection, host_id, deployed_list, deprecated_list):
    result = 0
    deployed_db = select_deployed_modules(db_connection, host_id)
    install_db = select_modules_to_install(db_connection, host_id, 1)
    install_id_list = [x[2] for x in install_db]
    # compare values to find host/db mismatches, while also updating table
    for module in deployed_list:
        if module["ID"] in deployed_db:
            update_deployed(
                db_connection,
                host_id,
                module["ID"],
                module["Count"],
                module["Activated"],
                module["ts"],
            )
            # we remove here to make list smaller and determine mismatches
            deployed_db.remove(module["ID"])
        else:
            # check if module was installed last cycle, if so update both tables
            if module["ID"] in install_id_list:
                req_install = 0
                update_built_module_install_requirement(
                    db_connection, host_id, module["ID"], req_install, module["ts"]
                )
                insert_deployed(
                    db_connection,
                    host_id,
                    module["ID"],
                    module["Count"],
                    module["Activated"],
                    cfg.SEC_WINDOW,
                    0,
                    module["ts"],
                    0,
                    0,
                )

                result = cfg.REFRESH_INSTALL_LIST
            else:
                # TODO: handle this case; maybe trigger revocation call
                id = module["ID"]
                # delete_built_module(db_connection, host_id, "IDS_server_MT")
                logger.info(
                    f"host has deployed module not in deployed or Install DB, module = {id}"
                )

    for module in deprecated_list:
        if module["ID"] in deployed_db:
            update_deployed(
                db_connection,
                host_id,
                module["ID"],
                module["Count"],
                module["Activated"],
                module["ts"],
            )
            # we remove here to make list smaller and determine mismatches
            deployed_db.remove(module["ID"])

    # anything left in deployed_db doesn't match report from host
    for module_id in deployed_db:
        logger.info(f"Module_id {module_id} in DB, but not reported by host")
        update_deployed_host_error(db_connection, host_id, module_id, int(time.time()))

    ###THIS IS WHERE WE NEED TO ADD THE IDS INFORMATION

    return result


def send_install_modules(conn_socket, host_id, modules):
    count = len(modules)
    for module in modules:
        logger.info(f"sending module {module}")
        command = {"cmd": "recv_module", "count": count}
        send_string = json.dumps(command, indent=4)
        conn_socket.sendall(bytes(send_string, encoding="utf-8"))
        data = conn_socket.recv(1024)
        data = data.decode("utf-8")
        if data != "Clear to send":
            logger.info("client can't accept")
            break
        else:
            send_ko_module(conn_socket, host_id, module)


def send_ko_module(conn_socket, host_id, module):
    mod_dir = cfg.nco_dir + cfg.symvers_dir + str(host_id) + "/modules/"

    filesize = os.path.getsize(mod_dir + module + ".ko")

    command = {"name": module + ".ko", "size": filesize}
    send_string = json.dumps(command, indent=4)
    conn_socket.sendall(bytes(send_string, encoding="utf-8"))

    # get ACK from host
    data = conn_socket.recv(1024)
    data = data.decode("utf-8")
    if data != "Clear to send":
        logger.info("client can't accept")
        return

    with open(mod_dir + module + ".ko", "rb") as file_to_send:
        logger.info(f"{module} file open")
        for data in file_to_send:
            # logger.info("sending module")
            conn_socket.sendall(data)
        logger.info(f"{module} file closed")


def retrieve_install_list(db_connection, host_id):
    install_list = select_modules_to_install(db_connection, host_id, 1)
    modules = [x[1] for x in install_list]
    return modules


def check_install_requirement_or_max_time(db_connection, host_id, end_time, interval):
    while int(time.time()) < end_time:
        # first check if new modules avail to install
        modules = retrieve_install_list(db_connection, host_id)
        if len(modules) > 0:
            return
        else:
            time.sleep(interval)


######################### DEPLOYED MODULE UPDATED FUNCTIONS #############################


# Handle toggling of modules; part of NCO controlled enabling of new module
def retrieve_toggle_active_list(db_connection, host_id):
    toggle_list = select_all_req_active_toggle(db_connection, host_id)
    if toggle_list == cfg.DB_ERROR:
        return -1
    mod_id = [x[1] for x in toggle_list]
    mode = [x[2] for x in toggle_list]
    return mod_id, mode


def toggle_active(conn_socket, db_connection, host_id, mod_id, mode):
    logger.info(f"Toggling module {mod_id} to {mode} for host {host_id}")
    # send active update command
    command = {"cmd": "toggle_active", "id": mod_id, "mode": mode}
    send_string = json.dumps(command, indent=4)
    conn_socket.sendall(bytes(send_string, encoding="utf-8"))
    try:
        data = conn_socket.recv(cfg.MAX_BUFFER_SIZE)
        json_data = json.loads(data)
    except json.decoder.JSONDecodeError as e:
        logger.info(f"Error on active mode update recv call\n {e}")
        return

    # Need to check for ERROR key if failed
    try:
        id = json_data["ID"]
        success = json_data["Result"]

        if success == 0:
            logger.info(f"Device error: {data}")
            return cfg.REVOKE_ERROR
        else:
            # remove module from require table; next report will handle table updates
            delete_req_active_toggle_by_id(db_connection, host_id, id)
            return 0
    except Exception as e:
        logger.info(f"Toggle exception: {e}")


# Handle toggling of modules; part of NCO controlled enabling of new module
def retrieve_set_priority_list(db_connection, host_id):
    priority_list = select_all_req_set_priority(db_connection, host_id)
    if priority_list == cfg.DB_ERROR:
        return -1
    if type(priority_list) != list:
        priority_list = []
    mod_id = [x[1] for x in priority_list]
    mode = [x[2] for x in priority_list]
    return mod_id, mode


def set_priority(conn_socket, db_connection, host_id, mod_id, priority):
    logger.info(f"Setting module {mod_id} priority to {priority} for host {host_id}")
    # send active update command
    command = {"cmd": "set_priority", "id": mod_id, "priority": priority}
    send_string = json.dumps(command, indent=4)
    conn_socket.sendall(bytes(send_string, encoding="utf-8"))
    try:
        data = conn_socket.recv(cfg.MAX_BUFFER_SIZE)
        json_data = json.loads(data)
    except json.decoder.JSONDecodeError as e:
        logger.info(f"Error on active mode update recv call\n {e}")
        return

    # Need to check for ERROR key if failed
    try:
        id = json_data["ID"]
        success = json_data["Result"]

        if success == 0:
            logger.info(f"Device error: {data}")
            return cfg.REVOKE_ERROR
        else:
            # remove module from require table; next report will handle table updates
            delete_req_set_priority_by_id(db_connection, host_id, id)
            return 0
    except Exception as e:
        logger.info(f"Prioirty exception: {e}")
