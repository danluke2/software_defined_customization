#!/usr/bin/env python3

import socket
import os
import json
import time

import cfg
from CIB_helper import *




######################### DEPLOY FUNCTIONS #############################

# Host reported deployed_list, which we compare to the deployed rows in our DB
# We also compare the deployed_list to the install rows and update both tables as necessary
def handle_deployed_update(db_connection, host_id, deployed_list):
    result = 0
    deployed_db = select_deployed_modules(db_connection, host_id)
    install_db = select_modules_to_install(db_connection, host_id, 1)
    install_id_list = [x[2] for x in install_db]
    #compare values to find host/db mismatches, while also updating table
    for module in deployed_list:
        if module["ID"] in deployed_db:
            update_deployed(db_connection, host_id, module["ID"], module["Count"], module["ts"])
            # we remove here to make list smaller and determine mismatches
            deployed_db.remove(module["ID"])
        else:
            #check if module was installed last cycle, if so update both tables
            if module["ID"] in install_id_list:
                req_install = 0
                update_built_module_install_requirement(db_connection, host_id, module["ID"], req_install, module["ts"])
                insert_deployed(db_connection, host_id, module["ID"], module["Count"], module["ts"], cfg.SEC_WINDOW, 0, 0)
                result = cfg.REFRESH_INSTALL_LIST
            else:
                # TODO: handle this case; maybe trigger revocation call
                print(f"host has deployed module not in deployed or Install DB, module =", module["ID"])

    for module_id in deployed_db:
        print(f"Module_id {module_id} in DB, but not reported by host")
        update_deployed_host_error(db_connection, host_id, module_id, int(time.time()))

    return result




def send_install_modules(conn_socket, host_id, modules):
    count = len(modules)
    for module in modules:
        print(f"sending module {module}")
        command = {"cmd": "recv_module", "count": count}
        send_string = json.dumps(command, indent=4)
        conn_socket.sendall(bytes(send_string,encoding="utf-8"))
        data = conn_socket.recv(1024)
        data = data.decode("utf-8")
        if data != 'Clear to send':
            print("client can't accept")
            break
        else:
            send_ko_module(conn_socket, host_id, module)



def send_ko_module(conn_socket, host_id, module):
    mod_dir = cfg.nco_dir + cfg.symvers_dir + str(host_id) + "/modules/"

    filesize = os.path.getsize(mod_dir + module + ".ko")

    command = {"name": module + ".ko", "size": filesize}
    send_string = json.dumps(command, indent=4)
    conn_socket.sendall(bytes(send_string,encoding="utf-8"))

    # get ACK from host
    data = conn_socket.recv(1024)
    data = data.decode("utf-8")
    if data != 'Clear to send':
        print("client can't accept")
        return

    with open(mod_dir + module + ".ko", 'rb') as file_to_send:
        print(f"{module} file open")
        for data in file_to_send:
            # print("sending module")
            conn_socket.sendall(data)
        print(f"{module} file closed")



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
