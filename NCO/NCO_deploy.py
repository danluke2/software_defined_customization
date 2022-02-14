#!/usr/bin/env python3

import socket
import os
import json
import time

import cfg
from CIB_helper import *




######################### DEPLOY FUNCTIONS #############################

def send_install_modules(conn_socket, host_id, modules):
    count = len(modules)
    for module in modules:
        print("sending module")
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
    mod_dir = symvers_dir + str(host_id) + "/modules/"

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
    # print(f"modules to install: {modules}")
    mod_ids = [x[2] for x in install_list]
    # print(f"module ids to install: {mod_ids}")

    return modules, mod_ids


def check_install_requirement_or_max_time(db_connection, host_id, end_time, interval):
    while int(time.time()) < end_time:
        # first check if new modules avail to install
        modules, module_ids = retrieve_install_list(db_connection, host_id)
        if len(modules) > 0:
            return
        else:
            time.sleep(interval)
