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

from exp_CIB_helper import *


parser = argparse.ArgumentParser(description='NCO test program')
parser.add_argument('--construct', help="Perform module constuction and deployment", action="store_true")
parser.add_argument('--sleep', type=int, required=False, help="Build thread sleep timer")
parser.add_argument('--ip', type=str, required=False, help="NCO IP")
parser.add_argument('--port', type=int, required=False, help="NCO port")
parser.add_argument('--number', type=int, required=True, help="Number of DCA threads")


args = parser.parse_args()

HOST = '10.0.0.20'
PORT = 65432        # Port to listen on (non-privileged ports are > 1023)
construct = "no"
timer = 30

if args.ip:
    HOST=args.ip

if args.port:
    PORT=args.port


if args.construct:
    construct = "y"

if args.sleep:
    timer = args.sleep


exp_mod_dir = "/home/vagrant/"
symvers_dir = exp_mod_dir + "device_modules/host_" # + host_id

next_module_id = 1

QUERY_INTERVAL = 30
INSERT_LINE = 42

REFRESH_INSTALL_LIST = 1
DB_ERROR = -1
CLOSE_SOCK = -2


######################### DEVICE THREAD FUNCTIONS #############################

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

    conn_socket.sendall(b'Clear to send')

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
    newPath = shutil.copy(exp_mod_dir + "software_defined_customization/experiment_modules/Makefile", symvers_dir + host_id + "/modules")
    #put a copy of common_structs in modules dir also
    newPath = shutil.copy(exp_mod_dir + "software_defined_customization/DCA_kernel/common_structs.h", symvers_dir + host_id + "/modules")


#mac is just the host test number, so reuse as host id to make simple
def handle_host_insert(db_connection, mac, ip, port, kernel_release):
    max_tries = 10
    counter = 0
    while counter <= max_tries:
        print(f"Inserting host {mac}, ID = {mac}")
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


def device_thread(conn, ip, port, cv, start_results, end_results, index, MAX_BUFFER_SIZE = 4096):
    db_connection = db_connect(exp_mod_dir + 'cib.db')

    #handle initial client-initiated check-in, then client is in a recv state

    try:
        #Client immediately sends a customization report upon connection
        data = conn.recv(MAX_BUFFER_SIZE)
        json_data = json.loads(data)
    except json.decoder.JSONDecodeError as e:
        print(f"Error on initial recv call, terminating connection\n {e}")
        conn.close()
        return

    device_mac = json_data["mac"]
    kernel_release = json_data["release"]

    host = select_host(db_connection, device_mac)

    if host == None:
        host = handle_host_insert(db_connection, device_mac, ip, port, kernel_release)

    if host == DB_ERROR:
        print("Host DB error occurred, terminating connection")
        conn.close()
        return

    # update host IP and Port if necessary
    if host["host_ip"] != ip:
        err = update_host(db_connection, device_mac, "host_ip", ip)
        if err == DB_ERROR:
            print(f"Host IP not updated for mac = {device_mac}")
    if host["host_port"] != port:
        err = update_host(db_connection, device_mac, "host_port", port)
        if err == DB_ERROR:
            print(f"Host Port not updated for mac = {device_mac}")


    host_id = host["host_id"]

    #get symver file and store in host_id dir if necessary
    if host["symvers_ts"] == 0:
        err = request_symver_file(conn, db_connection, host_id, device_mac)
        if err == DB_ERROR:
            print("Symver DB error occurred")
            return
        if err == CLOSE_SOCK:
            conn.close()
            return

    # initial check-in complete now wait for modules to be ready
    with cv:
        print(f"Host {host_id} waiting for builder to finish")
        cv.wait()
        start = int(time.time() * 1000)
        print(f"Host {host_id} start time = {start}")
        start_results[index] = start

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


    #get updated host report to ensure install success
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
            delta = end - start
            print(f"Host {host_id} delta time = {delta}")
            end_results[index] = end

    conn.close()
    return


###################### CONSTRUCTION THREAD FUNCTIONS ###########################


def insert_and_update_module_tables(db_connection, module, module_id, host_id):
    require_install = 1
    install_time = 0 #indicates ASAP
    print(f"inserting module {module} into built table")
    err = insert_built_module(db_connection, host_id, module, module_id, int(time.time()), 0, require_install, install_time)
    if err == DB_ERROR:
        print(f"Error inserting {module} into built table")
    return err



# we only build one module at a time, so module_id should not need race condition protection
# ASSUMPTION: symver file and directory exists
def build_ko_module(db_connection, host_id, module, module_id):
    try:
        subprocess.run(["./exp_builder.sh", module, str(module_id), str(INSERT_LINE), str(host_id)], check=True)
        result = 0
    except CalledProcessError as e:
        print(f"Error occured during module build process, error={e}")
        result = -1

    return result



# 1) Start construction thread and wait 't' seconds for each device DCA to connect
# to the NCO and deliver initial report and symver file
# 2) After all device check-ins completed, give option to clear the built module
# table, which will hold modules built during last experiment run.  This table
# does not need to be cleared for each trial
# 3) give option to build module for each device in the CIB.  After the build,
# we can verify all devices have an entry in BUILT table
# 4) Give option to built again, but likely select 'n'.  This alerts all the
# DCA threads that the modules are ready for deployment and the test begins
def construction_module_thread(cv, t, construct):
    db_connection = db_connect(exp_mod_dir + 'cib.db')
    # again = "y"
    # wait t time for all devices to connect and deliver symver file if needed
    time.sleep(t)
    with cv:
        #continuosly check if there are modules to build to host symvers
        # while again == "y":
        if construct =="y":
            # clear = input("Clear the built table (y/n)?")
            # if clear == "y":
            #clear built table
            drop_table(db_connection, "built_modules")
            #init table again
            init_built_modules_table(db_connection)

            # build = input("Build modules to each host (y/n)?")
            # if build == "y":
            # build_start = int(time.time() * 1000)
            host_list = select_all_hosts(db_connection)
            # get host_id from host_list
            hosts = [x[1] for x in host_list]
            for host in hosts:
                err = build_ko_module(db_connection, host, "nco_overhead_exp", host)
                if err == -1:
                    #move on to next module instead of updating the tables
                    print(f"Build module error for host {host}")
                    continue
                else:
                    print(f"Inserting module for host {host}")
                    err = insert_and_update_module_tables(db_connection, "nco_overhead_exp", host, host)
                    if err == DB_ERROR:
                        print(f"Error occured updating module tables")

            # build_end = int(time.time() * 1000)
            # diff = build_end - build_start
            # print(f"Build time was {diff} msec\n")
            # again = input(f"Build time was {diff} msec\n Build again?:")
            # else:
            #     again = "no"
        else:
            #reset modules for deployment
            host_list = select_all_hosts(db_connection)
            # get host_id from host_list
            hosts = [x[1] for x in host_list]
            for host in hosts:
                err = update_built_module_install_requirement(db_connection, host, host, 1, 0)
                if err == -1:
                    #move on to next module instead of updating the tables
                    print(f"Reset module error for host {host}")
                    continue

        cv.notifyAll()
    print("Construction module thread exiting")



if __name__ == "__main__":

    condition = Condition()
    counter = 0
    threads = []
    thread_start= [0 for x in range(args.number)]
    thread_end= [0 for x in range(args.number)]

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        try:
            s.bind((HOST, PORT))
            print('Socket bind complete')
        except socket.error as msg:
            print('Bind failed. Error : ' + str(sys.exc_info()))
            sys.exit()

        s.listen()

        while counter < args.number:
            conn, addr = s.accept()
            ip, port = str(addr[0]), str(addr[1])
            print('Accepting connection from ' + ip + ':' + port)
            try:
                t = Thread(target=device_thread, args=(conn, ip, port, condition, thread_start, thread_end, counter))
                t.start()
                threads.append(t)
                counter+=1
            except:
                print("Device thread creation error!")
                traceback.print_exc()

        # all threads expected have joined, so now construct modules if necessary
        try:
            #build module thread runs independent of client connections
            Thread(target=construction_module_thread, args=(condition,timer,construct)).start();
            print("Module construction thread running")
        except:
            print("Construction thread creation error!")
            traceback.print_exc()

        print("Joining device threads")
        for x in threads:
            x.join()

        minimum = 100000000000000000000000
        for element in thread_start:
            if element < minimum:
                minimum = element

        maximum = 0
        for element in thread_end:
            if element > maximum:
                maximum = element

        delta = maximum - minimum
        file = open(f"/home/vagrant/software_defined_customization/experiment_scripts/logs/nco_results_{args.number}.txt", 'a+')
        file.write(f"{delta}\n")
        file.close()
        file = open("/home/vagrant/software_defined_customization/experiment_scripts/logs/nco_finished.txt", 'w+')
        print("NCO finished file created")
        file.close()

        print(delta)
        s.close()
