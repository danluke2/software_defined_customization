#!/usr/bin/env python3

import socket
import os
import subprocess

import json

import random
import time

import shutil #for copy files
from threading import Thread
from threading import Condition
import traceback
import sys
import sqlite3 as sl

import argparse

from db_helper import *


HOST = '10.0.0.20'
PORT = 65432        # Port to listen on (non-privileged ports are > 1023)


core_mod_dir = "core_modules/"
symvers_dir = "device_modules/host_" # + host_id


build_interval = 10 #run build loop ever 10 seconds
next_module_id = 1

QUERY_INTERVAL = 30
INSERT_LINE = 42

REFRESH_INSTALL_LIST = 1
DB_ERROR = -1
CLOSE_SOCK = -2


parser = argparse.ArgumentParser(description='Process some integers.')
parser.add_argument('--t', type=int, required=True)

args = parser.parse_args()




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
        print("***************client can't accept********************")
        return

    with open(mod_dir + module + ".ko", 'rb') as file_to_send:
        print(f"{module} file open")
        for data in file_to_send:
            # print("sending module")
            conn_socket.sendall(data)
        print(f"{module} file closed")




######################### CLIENT THREAD FUNCTIONS #############################


def check_install_requirement_or_max_time(db_connection, host_id, end_time, interval):
    while int(time.time()) < end_time:
        # first check if new modules avail to install
        modules, module_ids = retrieve_install_list(db_connection, host_id)
        if len(modules) > 0:
            return
        else:
            time.sleep(interval)



def request_report(conn_socket, host_id):
    command = {"cmd": "run_report"}
    send_string = json.dumps(command, indent=4)
    conn_socket.sendall(bytes(send_string,encoding="utf-8"))
    # print(f"Sent report request to {host_id}")


def process_report(conn_socket, db_connection, host_id, MAX_BUFFER_SIZE):
    try:
        data = conn_socket.recv(MAX_BUFFER_SIZE)
        json_data = json.loads(data)
    except json.decoder.JSONDecodeError as e:
        print(f"Error on process report recv call, {e}")
        return CLOSE_SOCK
    # TODO: check report mac matches host_id and update DB?
    active_list = json_data["Active"]
    retired_list = json_data["Retired"]

    # update Active table based on retired list, if empty then for loop skips
    for module in retired_list:
        err = delete_active(db_connection, host_id, module["ID"] )
        if err == DB_ERROR:
            print(f"Active delete DB error occurred, host_id = {host_id}, module = {module}")
        else:
            err = insert_retired(db_connection, host_id, module["ID"], module["ts"])
            if err == DB_ERROR:
                print(f"Active delete DB error occurred, host_id = {host_id}, module = {module}")


    return active_list


def request_symver_file(conn_socket, db_connection, host_id, mac):
    command = {"cmd": "send_symvers"}
    send_string = json.dumps(command, indent=4)
    conn_socket.sendall(bytes(send_string,encoding="utf-8"))
    print("Sent symver request")
    err = recv_symvers_file(conn_socket, host_id)
    if err != CLOSE_SOCK:
        print("have symvers file")
        err = update_host(db_connection, mac, "symvers_ts", int(time.time()))
    return err


def recv_symvers_file(conn_socket, host_id):
    try:
        data = conn_socket.recv(1024)
        recv_string = json.loads(data)
    except json.decoder.JSONDecodeError as e:
        print(f"Error on recv symver recv call, {e}")
        return CLOSE_SOCK

    print('recv file: ', recv_string["file"])
    print('recv file size: ', recv_string["size"])
    file_size = recv_string["size"]

    with open(os.path.join(symvers_dir + str(host_id) + "/", "Module.symvers"), 'wb') as file_to_write:
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
                insert_active(db_connection, host_id, module["ID"], module["Count"], module["ts"], 0)
                result = REFRESH_INSTALL_LIST
            else:
                # TODO: handle this case; maybe trigger retire call
                print(f"host has active module not in Active or Install DB, module =", module["ID"])

    for module_id in active_db:
        print(f"Module_id {module_id} in DB, but not reported by host")
        update_active_host_error(db_connection, host_id, module_id, int(time.time()))

    return result


def retrieve_install_list(db_connection, host_id):
    install_list = select_modules_to_install(db_connection, host_id, 1)
    modules = [x[1] for x in install_list]
    # print(f"modules to install: {modules}")
    mod_ids = [x[2] for x in install_list]
    # print(f"module ids to install: {mod_ids}")

    return modules, mod_ids


# Build directory structure for storing host files
def setup_host_dirs(host_id):
    #create the host dir based on assigned host_id
    os.mkdir(symvers_dir + host_id)
    #create the modules dir based on assigned host_id
    os.mkdir(symvers_dir + host_id + "/modules")
    #put generic makefile in modules dir
    newPath = shutil.copy(core_mod_dir + "Makefile", symvers_dir + host_id + "/modules")



#mac is just the host test number, so reuse as host id to make simple
def handle_host_insert(db_connection, mac, ip, port, kernel_release):
    max_tries = 10
    counter = 0
    #repeatedly generate host ID and insert into db until successful
    while counter <= max_tries:
        # generated_id = random.randint(1,65535)
        print(f"Inserting host {mac}, ID = {mac}")
        # if host_id already exists, then DB error occurs and we try again
        err = insert_host(db_connection, mac, int(mac), ip, port, 0, 0, kernel_release, QUERY_INTERVAL)
        if (err == DB_ERROR):
            print("Could not insert host, try again")
            if counter == max_tries:
                return DB_ERROR
            counter +=1
            continue
        else:
            break
    setup_host_dirs(mac)
    host = select_host(db_connection, mac)
    if (host == DB_ERROR):
        print("Could not retrieve host after insert, DB_ERROR")
    return host


def client_thread(conn, ip, port, cv, MAX_BUFFER_SIZE = 4096):
    db_connection = db_connect('pcc.db')

    #handle initial client initiated check-in, then client is in a recv state

    try:
        #Client immediately sends a customization report upon connection
        data = conn.recv(MAX_BUFFER_SIZE)
        json_data = json.loads(data)
    except json.decoder.JSONDecodeError as e:
        print(f"Error on initial recv call, terminating connection\n {e}")
        conn.close()
        return

    client_mac = json_data["mac"]
    kernel_release = json_data["release"]


    host = select_host(db_connection, client_mac)

    if host == None:
        host = handle_host_insert(db_connection, client_mac, ip, port, kernel_release)

    if host == DB_ERROR:
        print("Host DB error occurred, terminating connection")
        conn.close()
        return

    # update host IP and Port if necessary
    if host["host_ip"] != ip:
        err = update_host(db_connection, client_mac, "host_ip", ip)
        if err == DB_ERROR:
            print(f"Host IP not updated for mac = {client_mac}")
    if host["host_port"] != port:
        err = update_host(db_connection, client_mac, "host_port", port)
        if err == DB_ERROR:
            print(f"Host Port not updated for mac = {client_mac}")


    host_id = host["host_id"]

    #get symver file and store in host_id dir if necessary
    if host["symvers_ts"] == 0:
        err = request_symver_file(conn, db_connection, host_id, client_mac)
        if err == DB_ERROR:
            print("Symver DB error occurred")

    # initial check-in complete now enter infinite loop while connection is active
    # all messages at this point are server driven
    with cv:
        print(f"Host {host_id} waiting for builder to finish")
        cv.wait()
        start = int(time.time() * 1000)
        print(f"Host {host_id} running, time = {start}")

    # get a full report from host and send updated modules
    request_report(conn, host_id)
    active_list = process_report(conn, db_connection, host_id, MAX_BUFFER_SIZE)
    if active_list == CLOSE_SOCK:
        print(f"Active list error, host {host_id}")
        conn.close()
        return

    #need install list to compare with active lists
    modules, module_ids = retrieve_install_list(db_connection, host_id)

    #update Active list based on active list from host, must compare also
    err = handle_active_update(db_connection, host_id, active_list, module_ids)
    if err == REFRESH_INSTALL_LIST:
        #need updated install list, but not the id's
        modules, module_ids = retrieve_install_list(db_connection, host_id)
        print(f"updated modules to install: {modules}")

    #send built modules to host
    send_install_modules(conn, host_id, modules)


    #get updated host report to ensure install success?
    request_report(conn, host_id)
    active_list = process_report(conn, db_connection, host_id, MAX_BUFFER_SIZE)
    if active_list == CLOSE_SOCK:
        print(f"Active list error, host {host_id}")
        conn.close()
        return

    if len(active_list) != 0:
        module = active_list[0]
        if module["ID"] != host_id:
            print(f"*************Module id error")
        else:
            end = int(time.time() * 1000)
            print(f"Host {host_id} done")

            print(f"End time = {end}")

    return


######################### MODULE THREAD FUNCTIONS #############################


#module has successfully been built, need to:
#   1. insert module into the built table
#   2. remove module from required build table - NOT for test scenario

def insert_and_update_module_tables(db_connection, module, module_id, host_id):
    require_install = 1
    install_time = 0 #indicates ASAP
    print(f"inserting module {module} into built table")
    err = insert_built_module(db_connection, host_id, module, module_id, int(time.time()), require_install, install_time)
    if err == DB_ERROR:
        print(f"Error inserting {module} into built table")
    return err



# we only build one module at a time, so module_id should not need race condition protection
# ASSUMPTION: symver file and directory exists
def build_ko_module(db_connection, host_id, module, module_id):
    try:
        subprocess.run(["./builder.sh", module, str(module_id), str(INSERT_LINE), str(host_id)], check=True)
        result = 0
    except CalledProcessError as e:
        print(f"Error occured during module build process, error={e}")
        result = -1

    return result


def build_module_thread(cv, t):
    db_connection = db_connect('pcc.db')
    again = "y"
    time.sleep(t)
    with cv:
        #continuosly check if there are modules to build to host symvers
        while again == "y":
            #clear built table
            # drop_table(db_connection, "built_modules")
            # #init table again
            # init_built_modules_table(db_connection)

            build = input("Built table cleared, press y to start test with build:")
            if build == "y":
                build_start = int(time.time() * 1000)
                host_list = select_all_hosts(db_connection)
                hosts = [x[1] for x in host_list]
                for host in hosts:
                    err = build_ko_module(db_connection, host, "pcc_test", host)
                    if err == -1:
                        #move on to next module instead of updating the tables
                        continue
                    else:
                        print(f"Inserting module for host {host}")
                        err = insert_and_update_module_tables(db_connection, "pcc_test", host, host)
                        if err == DB_ERROR:
                            print(f"Error occured updating module tables")


                build_end = int(time.time() * 1000)
                diff = build_end - build_start
                again = input(f"Build time was {diff} msec\n Build again?:")
            else:
                again = "no"

        cv.notifyAll()
    print("Build module thread exiting")



if __name__ == "__main__":

    condition = Condition()

    try:
        #build module thread runs independent of client connections
        Thread(target=build_module_thread, args=(condition,args.t)).start();
        print("Module build thread running")
    except:
        print("Client thread creation error!")
        traceback.print_exc()


    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        try:
            s.bind((HOST, PORT))
            print('Socket bind complete')
        except socket.error as msg:
            print('Bind failed. Error : ' + str(sys.exc_info()))
            sys.exit()

        s.listen()

        while True:
            conn, addr = s.accept()
            ip, port = str(addr[0]), str(addr[1])
            print('Accepting connection from ' + ip + ':' + port)
            try:
                Thread(target=client_thread, args=(conn, ip, port, condition)).start()
            except:
                print("Client thread creation error!")
                traceback.print_exc()
        s.close()