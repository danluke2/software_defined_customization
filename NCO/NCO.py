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

# remove prints and log instead
import logging

import cfg
from CIB_helper import *
from NCO_security import check_challenge, request_challenge_response
from NCO_construct import request_symver_file, construction_loop
from NCO_monitor import handle_host_insert, process_report, request_report
from NCO_deploy import send_install_modules, retrieve_install_list, check_install_requirement_or_max_time
from NCO_revoke import retrieve_revoke_list, revoke_module
from NCO_middlebox import update_inverse_module_requirements, middlebox_thread


parser = argparse.ArgumentParser(description='NCO program')
parser.add_argument('--ip', type=str, required=False, help="NCO IP")
parser.add_argument('--port', type=int, required=False, help="NCO port")
parser.add_argument('--middle_port', type=int, required=False, help="NCO middlebox port")
parser.add_argument('--build_interval', type=int, required=False, help="Construction interval")
parser.add_argument('--query_interval', type=int, required=False, help="DCA query interval")
parser.add_argument('--buffer', type=int, required=False, help="Max socket recv buffer")
parser.add_argument('--line', type=int, required=False, help="Construction module insert line")
parser.add_argument('--challenge', help="Perform module challenge/response", action="store_true")
parser.add_argument('--window', type=int, required=False, help="Security check window")
parser.add_argument('--linear', help="Assign host names in predictable fashion", action="store_true")
parser.add_argument('--logging', help="Enable logging to file instead of print to console", action="store_true" )
parser.add_argument('--logfile', type=str, required=False, help="Full log file path to use, defaults to layer4_5 directory")

args = parser.parse_args()


if args.ip:
    cfg.HOST=args.ip

if args.port:
    cfg.PORT=args.port

if args.middle_port:
    cfg.MIDDLE_PORT=args.middle_port

if args.build_interval:
    cfg.BUILD_INTERVAL=args.build_interval

if args.buffer:
    cfg.MAX_BUFFER_SIZE=args.buffer

if args.query_interval:
    cfg.QUERY_INTERVAL=args.query_interval

if args.line:
    cfg.INSERT_LINE=args.line

if args.challenge:
    cfg.SEC_WINDOW = 5 #default window
    if args.window:
        cfg.SEC_WINDOW = args.window

if args.linear:
    cfg.random_hosts = False

if args.logging:
    if args.logfile:
        logging.basicConfig(filename=args.logfile, level=logging.DEBUG)
    else:
        logging.basicConfig(filename=cfg.log_file, level=logging.DEBUG)
else:
    logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)




def construction_process(interval):
    logging.info("Construction: process running")
    db_connection = db_connect(cfg.nco_dir + 'cib.db')
    while(True):
        construction_loop(db_connection)
        time.sleep(interval)
        if exit_event.is_set():
            break

    logging.info("Construction: process exiting")




def middlebox_process(cv, interval):
    exit_event_mid = threading.Event()
    logging.info("Middlebox: process running")
    middle_threads = []
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        try:
            s.settimeout(5) # timeout for listening to check if shutdown condition reached
            s.bind((cfg.HOST, cfg.MIDDLE_PORT))
            logging.info('Middlebox: Socket bind complete')
        except socket.error as msg:
            logging.info('Middlebox: Bind failed. Error : ' + str(sys.exc_info()))
            sys.exit()

        s.listen()

        while True:
            if exit_event.is_set():
                exit_event_mid.set()
                logging.info("Middlebox: Joining middle threads")
                for x in middle_threads:
                    x.join()
                break
            try:
                (conn, (ip, port)) = s.accept()
                logging.info(f"Middlebox: Accepting middlebox connection from {ip}:{port}")
                middlebox = threading.Thread(target=middlebox_thread, args=(conn, ip, port, cv, cfg.MAX_BUFFER_SIZE, interval, exit_event_mid))
                middlebox.start()
                middle_threads.append(middlebox)
            except socket.timeout:
                pass
            except:
                logging.info("Middlebox: thread creation error!")
                traceback.print_exc()

        s.close()

    logging.info("Middlebox: process exiting")




def device_thread(conn, ip, port, buffer_size, interval):
    try:
        db_connection = db_connect(cfg.nco_dir + 'cib.db')

        #handle initial device initiated check-in, then device is in a recv state
        try:
            #Device immediately sends a customization report upon connection
            data = conn.recv(buffer_size)
            json_data = json.loads(data)
        except json.decoder.JSONDecodeError as e:
            logging.info(f"Device: Error on initial recv call, terminating connection\n {e}")
            conn.close()
            return

        device_mac = json_data["mac"]
        kernel_release = json_data["release"]

        host = select_host(db_connection, device_mac)

        if host == None:
            host = handle_host_insert(db_connection, device_mac, ip, port, kernel_release, interval)

        if host == cfg.DB_ERROR:
            logging.info("Device: Host DB error occurred, terminating connection")
            conn.close()
            return

        # update host IP and Port if necessary
        if host["host_ip"] != ip:
            err = update_host(db_connection, device_mac, "host_ip", ip)
            if err == cfg.DB_ERROR:
                logging.info(f"Device: Host IP not updated for mac = {device_mac}")
        if host["host_port"] != port:
            err = update_host(db_connection, device_mac, "host_port", port)
            if err == cfg.DB_ERROR:
                logging.info(f"Device: Host Port not updated for mac = {device_mac}")


        # initial check-in complete now enter infinite loop while connection is active
        # all messages at this point are server driven

        while True:
            if exit_event.is_set():
                conn.close()
                return
            host_id = host["host_id"]

            #Construction requirement: get symver file and store in host_id dir if necessary
            if host["symvers_ts"] == 0:
                err = request_symver_file(conn, db_connection, host_id, device_mac)
                if err == cfg.DB_ERROR:
                    logging.info("Device: Symver DB error occurred")
                    continue
                if err == cfg.CLOSE_SOCK:
                    conn.close()
                    break


            #Revoke requirement: get revoke list before getting report
            revoke_id_list, revoke_name_list = retrieve_revoke_list(db_connection, host_id)
            if len(revoke_id_list) > 0:
                for i in range(len(revoke_id_list)):
                    revoke_module(conn, db_connection, host_id, revoke_id_list[i], revoke_name_list[i])


            #Monitor requirement: get a full report from host and update CIB
            request_report(conn, host_id)
            err = process_report(conn, db_connection, host_id, buffer_size)
            if err == cfg.CLOSE_SOCK:
                conn.close()
                break


            #Deploy requirement: need install list for host_id
            modules = retrieve_install_list(db_connection, host_id)

            if len(modules) > 0:
                #send built modules to host
                send_install_modules(conn, host_id, modules)
                # get a full report from host since we installed new modules
                request_report(conn, host_id)
                err = process_report(conn, db_connection, host_id, buffer_size)
                if err == cfg.CLOSE_SOCK:
                    conn.close()
                    break

                #Middlebox requirement: update inverse module table (if necessary)
                update_inverse_module_requirements(db_connection, modules)

            #Security requirement: challenge deployed modules
            if args.challenge:
                #get challenge list and send challenges
                challenge_list = check_challenge(db_connection, host_id)
                for mod_id in challenge_list:
                    temp = request_challenge_response(conn, db_connection, host_id, mod_id, buffer_size)
                    if temp == cfg.CLOSE_SOCK:
                        conn.close()
                        break
                    # if temp == cfg.RETIRE_MOD:
                    if temp == cfg.REVOKE_MOD:
                        logging.info("Device: ************ Failed challenge/respnse, Revoking module************")
                        #send revoke command b/c module failed check
                        err = revoke_module(conn, db_connection, host_id, mod_id)
                        if err != 0:
                            logging.info(f"Device: revoke module error")
                    else:
                        #update challenge ts
                        now = int(time.time())
                        update_deployed_sec_ts(db_connection, host_id, mod_id, now)

            #refresh host value each cycle in case DB updated
            host = select_host(db_connection, device_mac)

            if host == cfg.DB_ERROR:
                logging.info("Device: Host DB error occurred, terminating connection")
                conn.close()
                break

            else:
                logging.info("Device: Entering a wait state to interact with host again\n\n")
                start_time = int(time.time())
                end_time = start_time + host["interval"]
                interval = int(host["interval"] / 5)
                check_install_requirement_or_max_time(db_connection, host_id, end_time, interval)
    except:
        logging.info("Device: thread exception")
        traceback.print_exc()
        conn.close()



def signal_handler(signum, frame):
    logging.info("SIGINT: handler called")
    exit_event.set()
    sys.exit()



if __name__ == "__main__":
    exit_event = threading.Event()
    signal.signal(signal.SIGINT, signal_handler)
    condition = threading.Condition()
    device_threads = []

    try:
        #construction process runs independent of device connections
        construct = multiprocessing.Process(target=construction_process, name="constuction", args=(cfg.BUILD_INTERVAL,))
        construct.start()

    except:
        logging.info("Construction: process creation error!")
        traceback.print_exc()

    try:
        #middlebox process runs independent of device connections
        middlebox = multiprocessing.Process(target=middlebox_process, name="middlebox", args=(condition, cfg.BUILD_INTERVAL,))
        middlebox.start()

    except:
        logging.info("Middlebox: process creation error!")
        traceback.print_exc()


    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        try:
            s.settimeout(5) # timeout for listening to check if shutdown condition reached
            s.bind((cfg.HOST, cfg.PORT))
            logging.info('Socket bind complete')
        except socket.error as msg:
            logging.info('Bind failed. Error : ' + str(sys.exc_info()))
            sys.exit()


        s.listen()

        while True:
            if exit_event.is_set():
                break
            try:
                (conn, (ip, port)) = s.accept()
                logging.info(f"Accepting connection from {ip}:{port}")
                device = threading.Thread(target=device_thread, args=(conn, ip, port, cfg.MAX_BUFFER_SIZE, cfg.QUERY_INTERVAL))
                device.start()
                device_threads.append(device)
            except socket.timeout:
                pass
            except:
                logging.info("Device: thread creation error!")
                traceback.print_exc()

        s.close()
        logging.info("Joining device threads")
        for x in device_threads:
            x.join()
        exit_event.set()
