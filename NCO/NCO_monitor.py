#!/usr/bin/env python3

import socket
import os
import json
import random
import shutil  # for copy files

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


def process_report(conn_socket, db_connection, host_id, buffer_size):
    try:
        data = conn_socket.recv(buffer_size)
        json_data = json.loads(data)
    except json.decoder.JSONDecodeError as e:
        logger.info(f"Error on process report recv call, {e}")
        return cfg.CLOSE_SOCK
    # TODO: check report mac matches host_id and update DB?
    deployed_list = json_data["Active"]
    deprecated_list = json_data["Deprecated"]
    revoked_list = json_data["Revoked"]

    # update deployed table based on revoked list
    handle_revoke_update(db_connection, host_id, revoked_list)
    # update deployed table based on active list
    handle_deployed_update(db_connection, host_id,
                           deployed_list, deprecated_list)
    return 0


# Build directory structure for storing host files
def setup_host_dirs(host_id):
    # create the host dir based on assigned host_id
    os.mkdir(cfg.nco_dir + cfg.symvers_dir + host_id)
    # create the modules dir based on assigned host_id
    os.mkdir(cfg.nco_dir + cfg.symvers_dir + host_id + "/modules")
    # put generic makefile in modules dir
    newPath = shutil.copy(cfg.nco_mod_dir + "Makefile",
                          cfg.nco_dir + cfg.symvers_dir + host_id + "/modules")
    # put a copy of common_structs in host dir for all modules
    newPath = shutil.copy(cfg.common_struct_dir +
                          "common_structs.h", cfg.nco_dir + cfg.symvers_dir + host_id)


def handle_host_insert(db_connection, mac, ip, port, kernel_release, interval):
    max_tries = 10
    counter = 0
    # repeatedly generate host ID and insert into db until successful
    while counter <= max_tries:
        if cfg.random_hosts:
            generated_id = random.randint(1, 65535)
        else:
            generated_id = cfg.next_module_id
            cfg.next_module_id += 1

        logger.info(f"Inserting host {mac}, ID = {generated_id}")
        # if host_id already exists, then DB error occurs and we try again
        err = insert_host(db_connection, mac, generated_id,
                          ip, port, 0, 0, kernel_release, interval)
        if (err == cfg.DB_ERROR):
            logger.info("Could not insert host, try again")
            if counter == max_tries:
                return cfg.DB_ERROR
            counter += 1
            continue
        else:
            break
    setup_host_dirs(str(generated_id))
    host = select_host(db_connection, mac)
    if (host == cfg.DB_ERROR):
        logger.info("Could not retrieve host after insert, DB_ERROR")
    return host


# Handle toggling of modules; part of NCO controlled enabling of new module
def retrieve_toggle_list(db_connection, host_id):
    toggle_list = select_all_req_toggle(db_connection, host_id)
    if toggle_list == cfg.DB_ERROR:
        return -1
    mod_id = [x[1] for x in toggle_list]
    mode = [x[2] for x in toggle_list]
    return mod_id, mode


def toggle_module(conn_socket, db_connection, host_id, mod_id, mode):
    logger.info(f"Toggling module {mod_id} to {mode} for host {host_id}")
    # send toggle command
    command = {"cmd": "toggle_module", "id": mod_id, "mode": mode}
    send_string = json.dumps(command, indent=4)
    conn_socket.sendall(bytes(send_string, encoding="utf-8"))
    try:
        data = conn_socket.recv(cfg.MAX_BUFFER_SIZE)
        json_data = json.loads(data)
    except json.decoder.JSONDecodeError as e:
        logger.info(f"Error on toggle recv call\n {e}")
        return

    # Need to check for ERROR key if failed
    try:
        id = json_data["ID"]
        success = json_data["Result"]

        if success == 0:
            logger.info(f"Device error: {data}")
            return cfg.REVOKE_ERROR
        else:
            # remove module from require toggle table; next report will handle table updates
            delete_req_toggle_by_id(db_connection, host_id, id)
            return 0
    except Exception as e:
        logger.info(f"Toggle exception: {e}")
