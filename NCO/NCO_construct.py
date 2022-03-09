#!/usr/bin/env python3

import subprocess
import time
import json
import os

import cfg
from CIB_helper import *
from NCO_security import generate_key



######################### CONSTRUCTION FUNCTION ################################

# Need symver file from host to allow building modules remotely since each
# module calls exported L4.5 DCA functions
def request_symver_file(conn_socket, db_connection, host_id, mac):
    command = {"cmd": "send_symvers"}
    send_string = json.dumps(command, indent=4)
    conn_socket.sendall(bytes(send_string,encoding="utf-8"))
    print("Sent symver request")
    err = recv_symvers_file(conn_socket, host_id)
    if err != cfg.CLOSE_SOCK:
        print("have symvers file")
        err = update_host(db_connection, mac, "symvers_ts", int(time.time()))
    return err


def recv_symvers_file(conn_socket, host_id):
    # 2 stages: 1st -> filename and size; 2nd->file
    try:
        data = conn_socket.recv(1024)
        recv_string = json.loads(data)
    except json.decoder.JSONDecodeError as e:
        print(f"Error on recv symver recv call, {e}")
        print(f"recv: {data}")
        return cfg.CLOSE_SOCK

    print('recv file: ', recv_string["file"])
    print('recv file size: ', recv_string["size"])
    file_size = recv_string["size"]

    conn_socket.sendall(b'Clear to send')

    with open(os.path.join(cfg.git_dir + cfg.symvers_dir + str(host_id) + "/", "Module.symvers"), 'wb') as file_to_write:
        while True:
            data = conn_socket.recv(file_size)
            if not data:
                break
            file_to_write.write(data)
            file_size -= len(data)
            print("remaining = ", file_size)
            if file_size<=0:
                break
        file_to_write.close()



#module has successfully been built, need to:
#   1. insert module into the built table
#   2. remove module from required build table
def insert_and_update_module_tables(db_connection, module, module_id, host_id, key):
    require_install = 1
    install_time = 0 #indicates ASAP
    print(f"inserting module {module} into built table")
    err = insert_built_module(db_connection, host_id, module, module_id, int(time.time()), key, require_install, install_time)
    if err == cfg.DB_ERROR:
        print(f"Error inserting {module} into built table")
    else:
        print(f"Deleting module from required build table")
        err = delete_req_build_module(db_connection, module, host_id)
        if err == cfg.DB_ERROR:
            print(f"Error removing module {module} from required build table")
            #TODO: what action should be taken in this case?

    return err



# we only build one module at a time, so mod_id should not need race condition protection
# ASSUMPTION: symver file and directory exists
def build_ko_module(db_connection, host_id, module, module_id, key):
    try:
        subprocess.run([cfg.git_dir + "builder.sh", module, str(module_id), str(cfg.INSERT_LINE), str(host_id), key.hex()], check=True)
        result = 0
    except subprocess.CalledProcessError as e:
        print(f"Error occured during module build process, error={e}")
        result = -1

    return result


def construction_loop(db_connection):
    #continuosly check if there are modules to build to host symvers
    modules_to_build = select_all_req_build_modules(db_connection)
    host_id_list = [x[0] for x in modules_to_build]
    module_list = [x[1] for x in modules_to_build]
    for i in range(len(host_id_list)):
        key = generate_key()
        err = build_ko_module(db_connection, host_id_list[i], module_list[i], cfg.next_module_id, key)
        if err == -1:
            #move on to next module instead of updating the tables
            print(f"Construction error for host {host_id_list[i]}")
            continue
        else:
            err = insert_and_update_module_tables(db_connection, module_list[i], cfg.next_module_id, host_id_list[i], key)
            if err == cfg.DB_ERROR:
                print(f"Error occured updating module tables")
            cfg.next_module_id += 1
