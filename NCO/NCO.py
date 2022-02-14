#!/usr/bin/env python3

import socket
import json
import time
import traceback
import sys
import argparse
import signal
import threading
import multiprocessing


import cfg
from CIB_helper import *
from NCO_security import *
from NCO_construct import *
from NCO_monitor import *
from NCO_deploy import *


parser = argparse.ArgumentParser(description='NCO program')
parser.add_argument('--ip', type=str, required=False, help="NCO IP")
parser.add_argument('--port', type=int, required=False, help="NCO port")
parser.add_argument('--interval', type=int, required=False, help="Construction interval")
parser.add_argument('--buffer', type=int, required=False, help="Max socket recv buffer")
parser.add_argument('--query', type=int, required=False, help="DCA query interval")
parser.add_argument('--line', type=int, required=False, help="Construction module insert line")
parser.add_argument('--challenge', help="Perform module challenge/response", action="store_true")
parser.add_argument('--window', type=int, required=False, help="Security check window")

args = parser.parse_args()


if args.ip:
    cfg.HOST=args.ip

if args.port:
    cfg.PORT=args.port

if args.interval:
    cfg.BUILD_INTERVAL=args.interval

if args.buffer:
    cfg.MAX_BUFFER_SIZE=args.buffer

if args.query:
    cfg.QUERY_INTERVAL=args.query

if args.line:
    cfg.INSERT_LINE=args.line

if args.challenge:
    cfg.SEC_WINDOW = 5 #default window
    if args.window:
        cfg.SEC_WINDOW = args.window






def construction_process(interval):
    print("Construction process running")
    db_connection = db_connect('cib.db')
    while(True):
        construction_loop(db_connection)
        time.sleep(interval)
        if exit_event.is_set():
            break

    print("Construction process exiting")



def device_thread(conn, ip, port, buffer_size, interval):
    try:

        db_connection = db_connect('cib.db')

        #handle initial device initiated check-in, then device is in a recv state
        try:
            #Device immediately sends a customization report upon connection
            data = conn.recv(buffer_size)
            json_data = json.loads(data)
        except json.decoder.JSONDecodeError as e:
            print(f"Error on initial recv call, terminating connection\n {e}")
            conn.close()
            return

        device_mac = json_data["mac"]
        kernel_release = json_data["release"]

        host = select_host(db_connection, device_mac)

        if host == None:
            host = handle_host_insert(db_connection, device_mac, ip, port, kernel_release, interval)

        if host == cfg.DB_ERROR:
            print("Host DB error occurred, terminating connection")
            conn.close()
            return

        # update host IP and Port if necessary
        if host["host_ip"] != ip:
            err = update_host(db_connection, device_mac, "host_ip", ip)
            if err == cfg.DB_ERROR:
                print(f"Host IP not updated for mac = {device_mac}")
        if host["host_port"] != port:
            err = update_host(db_connection, device_mac, "host_port", port)
            if err == cfg.DB_ERROR:
                print(f"Host Port not updated for mac = {device_mac}")


        # initial check-in complete now enter infinite loop while connection is active
        # all messages at this point are server driven

        while True:
            host_id = host["host_id"]

            #get symver file and store in host_id dir if necessary
            if host["symvers_ts"] == 0:
                err = request_symver_file(conn, db_connection, host_id, device_mac)
                if err == cfg.DB_ERROR:
                    print("Symver DB error occurred")
                    continue
                if err == cfg.CLOSE_SOCK:
                    conn.close()
                    break


            # get a full report from host and send updated modules
            request_report(conn, host_id)
            active_list = process_report(conn, db_connection, host_id, buffer_size)
            if active_list == cfg.CLOSE_SOCK:
                conn.close()
                break

            #challenge active module testing
            if len(active_list) > 0:
                res = input("Active list not empty, press y to start challeng test:")
                if res == "y":
                    temp = request_challenge_response(conn, db_connection, host_id, active_list, buffer_size)


            #need install list to compare with active lists
            modules, module_ids = retrieve_install_list(db_connection, host_id)

            #update Active list based on active list from host, must compare also
            err = handle_active_update(db_connection, host_id, active_list, module_ids)
            if err == cfg.REFRESH_INSTALL_LIST:
                #need updated install list, but not the id's
                modules, module_ids = retrieve_install_list(db_connection, host_id)
                print(f"updated modules to install: {modules}")

            if len(modules) > 0:
                #send built modules to host
                send_install_modules(conn, host_id, modules)

            #refresh host value each cycle in case DB updated
            host = select_host(db_connection, device_mac)

            if host == cfg.DB_ERROR:
                print("Host DB error occurred, terminating connection")
                conn.close()
                break

            else:
                print("Entering a wait state to interact with host again\n\n")
                start_time = int(time.time())
                end_time = start_time + host["interval"]
                interval = int(host["interval"] / 5)
                check_install_requirement_or_max_time(db_connection, host_id, end_time, interval)
    except:
        print("Device thread exception")
        traceback.print_exc()
        conn.close()



def signal_handler(signum, frame):
    print("SIGINT handler called")
    exit_event.set()
    s.close()



if __name__ == "__main__":
    exit_event = threading.Event()
    signal.signal(signal.SIGINT, signal_handler)

    try:
        #construction process runs independent of device connections
        construct = multiprocessing.Process(target=construction_process, name="constuction", args=(cfg.BUILD_INTERVAL,))
        construct.start()

    except:
        print("Construction process creation error!")
        traceback.print_exc()



    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        try:
            s.bind((cfg.HOST, cfg.PORT))
            print('Socket bind complete')
        except socket.error as msg:
            print('Bind failed. Error : ' + str(sys.exc_info()))
            sys.exit()


        s.listen()

        while True:
            conn, addr = s.accept()
            ip, port = str(addr[0]), str(addr[1])
            print(f"Accepting connection from {ip}:{port}")
            try:
                device = threading.Thread(target=device_thread, args=(conn, ip, port, cfg.MAX_BUFFER_SIZE, cfg.QUERY_INTERVAL))
                device.start()
            except:
                print("Device thread creation error!")
                traceback.print_exc()

        s.close()
        exit_event.set()
