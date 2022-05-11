#!/usr/bin/env python3

import socket
import json
import time

import cfg
from CIB_helper import *

import logging
import sys


logger = logging.getLogger(__name__)  # use module name



######################### REVOKE FUNCTIONS #############################

def handle_revoke_update(db_connection, host_id, revoked_list):
    # update deployed table based on revoked list, if empty then for loop skips
    for module in revoked_list:
        err = delete_deployed(db_connection, host_id, module["ID"] )
        if err == cfg.DB_ERROR:
            logger.info(f"deployed delete DB error occurred, host_id = {host_id}, module = {module}")
        else:
            err = insert_revoked(db_connection, host_id, module["ID"], module["ts"])
            if err == cfg.DB_ERROR:
                logger.info(f"deployed delete DB error occurred, host_id = {host_id}, module = {module}")



def retrieve_revoke_list(db_connection, host_id):
    revoke_list = select_all_req_revocation(db_connection, host_id)
    if revoke_list == cfg.DB_ERROR:
        return -1
    mod_id = [x[1] for x in revoke_list]
    mod_name = [x[2] for x in revoke_list]
    return mod_id, mod_name



def revoke_module(conn_socket, db_connection, host_id, mod_id, name = ''):
    if name == '':
        name = select_built_module_by_id(db_connection, host_id, mod_id)
        if name == cfg.DB_ERROR:
            return cfg.DB_ERROR
        module = name["module"]
    else:
        module = name
    logger.info(f"revoking module {module}")
    # send revoke command
    command = {"cmd": "revoke_module", "name": module}
    send_string = json.dumps(command, indent=4)
    conn_socket.sendall(bytes(send_string,encoding="utf-8"))
    data = conn_socket.recv(cfg.MAX_BUFFER_SIZE)
    data = data.decode("utf-8")
    if data != 'success':
        logger.info(f"Device error: {data}")
        return cfg.REVOKE_ERROR
    else:
        # remove module from require revoke table; next report will handle revoked table update
        delete_req_revocation_by_id(db_connection, host_id, mod_id)
        return 0



# Handle deactivation of modules part of staged transition
def retrieve_deactivate_list(db_connection, host_id):
    deactivate_list = select_all_req_deactivate(db_connection, host_id)
    if deactivate_list == cfg.DB_ERROR:
        return -1
    mod_id = [x[1] for x in deactivate_list]
    return mod_id



def deactivate_module(conn_socket, db_connection, host_id, mod_id):
    logger.info(f"Deactivating module {mod_id} for host {host_id}")
    # send deactivate command
    command = {"cmd": "deactivate_module", "id": mod_id}
    send_string = json.dumps(command, indent=4)
    conn_socket.sendall(bytes(send_string,encoding="utf-8"))
    try:
        #Device immediately sends a customization report upon connection
        data = conn_socket.recv(cfg.MAX_BUFFER_SIZE)
        json_data = json.loads(data)
    except json.decoder.JSONDecodeError as e:
        logger.info(f"Error on deactivate recv call\n {e}")
        return

    try:
        id = json_data["ID"]
        success = json_data["Result"]

        if success == 0:
            logger.info(f"Device error: {data}")
            return cfg.REVOKE_ERROR
        else:
            # remove module from require deactivate table; next report will handle  table updates
            delete_req_deactivate_by_id(db_connection, host_id, id)
            return 0
    except Exception as e:
        logger.info(f"Deactivate exception: {e}")
