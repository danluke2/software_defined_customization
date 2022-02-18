#!/usr/bin/env python3

import socket
import os
import json
import random
import shutil #for copy files

import cfg
from CIB_helper import *


######################### MONITORING FUNCTIONS #############################


def request_report(conn_socket, host_id):
    command = {"cmd": "run_report"}
    send_string = json.dumps(command, indent=4)
    conn_socket.sendall(bytes(send_string,encoding="utf-8"))
    print(f"Sent report request to {host_id}")



def process_report(conn_socket, db_connection, host_id, buffer_size):
    try:
        data = conn_socket.recv(buffer_size)
        json_data = json.loads(data)
    except json.decoder.JSONDecodeError as e:
        print(f"Error on process report recv call, {e}")
        return cfg.CLOSE_SOCK
    # TODO: check report mac matches host_id and update DB?
    active_list = json_data["Active"]
    retired_list = json_data["Retired"]

    # update Active table based on retired list, if empty then for loop skips
    for module in retired_list:
        err = delete_active(db_connection, host_id, module["ID"] )
        if err == cfg.DB_ERROR:
            print(f"Active delete DB error occurred, host_id = {host_id}, module = {module}")
        else:
            err = insert_retired(db_connection, host_id, module["ID"], module["ts"])
            if err == cfg.DB_ERROR:
                print(f"Active delete DB error occurred, host_id = {host_id}, module = {module}")

    return active_list



# Host reported active_list, which we compare to the active rows in our DB
# We also compare the active_list to the install rows and update both tables as necessary
def handle_active_update(db_connection, host_id, active_list, install_id_list):
    result = 0
    active_db = select_active_modules(db_connection, host_id)
    # print(f"Active list {active_list}")
    # print(f"Install id list {install_id_list}")
    #compare values to find host/db mismatches, while also updating table
    for module in active_list:
        if module["ID"] in active_db:
            update_active(db_connection, host_id, module["ID"], module["Count"], module["ts"])
            # we remove here to make list smaller and determine mismatches
            active_db.remove(module["ID"])
        else:
            #check if module was installed last cycle, if so update both tables
            if module["ID"] in install_id_list:
                req_install = 0
                update_built_module_install_requirement(db_connection, host_id, module["ID"], req_install, module["ts"])
                insert_active(db_connection, host_id, module["ID"], module["Count"], module["ts"], cfg.SEC_WINDOW, 0, 0)
                result = cfg.REFRESH_INSTALL_LIST
            else:
                # TODO: handle this case; maybe trigger retire call
                print(f"host has active module not in Active or Install DB, module =", module["ID"])

    for module_id in active_db:
        print(f"Module_id {module_id} in DB, but not reported by host")
        update_active_host_error(db_connection, host_id, module_id, int(time.time()))

    return result



# Build directory structure for storing host files
def setup_host_dirs(host_id):
    #create the host dir based on assigned host_id
    os.mkdir(cfg.symvers_dir + host_id)
    #create the modules dir based on assigned host_id
    os.mkdir(cfg.symvers_dir + host_id + "/modules")
    #put generic makefile in modules dir
    newPath = shutil.copy(cfg.core_mod_dir + "Makefile", cfg.symvers_dir + host_id + "/modules")
    #put a copy of common_structs in host dir for all modules
    newPath = shutil.copy(cfg.core_mod_dir + "common_structs.h", cfg.symvers_dir + host_id)


def handle_host_insert(db_connection, mac, ip, port, kernel_release, interval):
    max_tries = 10
    counter = 0
    #repeatedly generate host ID and insert into db until successful
    while counter <= max_tries:
        generated_id = random.randint(1,65535)
        print(f"Inserting host {mac}, ID = {generated_id}")
        # if host_id already exists, then DB error occurs and we try again
        err = insert_host(db_connection, mac, generated_id, ip, port, 0, 0, kernel_release, interval)
        if (err == cfg.DB_ERROR):
            print("Could not insert host, try again")
            if counter == max_tries:
                return cfg.DB_ERROR
            counter +=1
            continue
        else:
            break
    setup_host_dirs(str(generated_id))
    host = select_host(db_connection, mac)
    if (host == cfg.DB_ERROR):
        print("Could not retrieve host after insert, DB_ERROR")
    return host
