#!/usr/bin/env python3

import socket
import json
import time
import traceback
import sys
import argparse
import signal
import subprocess
import threading
import multiprocessing

# remove prints and log instead
import logging
from logging import handlers

import cfg
from CIB_helper import *


from NCO_logging import logger_process
from NCO_security import check_challenge, request_challenge_response
from NCO_construct import request_symver_file, construction_process
from NCO_monitor import handle_host_insert, process_report, request_report
from NCO_deploy import (
    send_install_modules,
    retrieve_install_list,
    check_install_requirement_or_max_time,
    retrieve_toggle_active_list,
    toggle_active,
    retrieve_set_priority_list,
    set_priority,
)
from NCO_revoke import (
    retrieve_revoke_list,
    revoke_module,
    retrieve_deprecated_list,
    deprecate_module,
)
from NCO_middlebox import update_inverse_module_requirements, middlebox_process


parser = argparse.ArgumentParser(description="NCO program")
parser.add_argument("--ip", type=str, required=False, help="NCO IP")
parser.add_argument("--port", type=int, required=False, help="NCO port")
parser.add_argument(
    "--middle_port", type=int, required=False, help="NCO middlebox port"
)
parser.add_argument(
    "--build_interval", type=int, required=False, help="Construction interval"
)
parser.add_argument(
    "--query_interval", type=int, required=False, help="DCA query interval"
)
parser.add_argument("--buffer", type=int, required=False, help="Max socket recv buffer")
parser.add_argument(
    "--challenge", help="Perform module challenge/response", action="store_true"
)
parser.add_argument("--window", type=int, required=False, help="Security check window")
parser.add_argument(
    "--linear", help="Assign host names in predictable fashion", action="store_true"
)
parser.add_argument(
    "--host_name_ip", help="Assign host names based on IP address", action="store_true"
)
parser.add_argument("--print", help="Enables logging to console", action="store_true")
parser.add_argument(
    "--logfile",
    type=str,
    required=False,
    help="Full log file path to use, defaults to layer4_5 directory",
)

args = parser.parse_args()


if args.ip:
    cfg.HOST = args.ip

if args.port:
    cfg.PORT = args.port

if args.middle_port:
    cfg.MIDDLE_PORT = args.middle_port

if args.build_interval:
    cfg.BUILD_INTERVAL = args.build_interval

if args.buffer:
    cfg.MAX_BUFFER_SIZE = args.buffer

if args.query_interval:
    cfg.QUERY_INTERVAL = args.query_interval

if args.challenge:
    cfg.SEC_WINDOW = 5  # default window
    if args.window:
        cfg.SEC_WINDOW = args.window

if args.linear:
    cfg.random_hosts = False

if args.host_name_ip:
    cfg.random_hosts = False
    cfg.ip_hosts = True

if args.print:
    cfg.log_console = True

if args.logfile:
    cfg.log_file = args.logfile


logger = logging.getLogger(__name__)  # use module name


def root_configurer(queue):
    h = handlers.QueueHandler(queue)
    root = logging.getLogger()
    root.addHandler(h)
    root.setLevel(logging.DEBUG)


def device_thread(conn, ip, port, buffer_size, interval):
    try:
        db_connection = db_connect(cfg.nco_dir + "cib.db")

        # handle device initiated check-in, then device is in a recv state
        try:
            # Device immediately sends a customization report upon connection
            data = conn.recv(buffer_size)
            json_data = json.loads(data)
        except json.decoder.JSONDecodeError as e:
            logger.info(f"Error on initial recv call, terminating connection\n {e}")
            conn.close()
            return

        device_mac = json_data["mac"]
        kernel_release = json_data["release"]

        host = select_host(db_connection, device_mac)

        if host == None:
            host = handle_host_insert(
                db_connection, device_mac, ip, port, kernel_release, interval
            )

        if host == cfg.DB_ERROR:
            logger.info("Host DB error occurred, terminating connection")
            conn.close()
            return

        # update host IP and Port if necessary
        if host["host_ip"] != ip:
            err = update_host(db_connection, device_mac, "host_ip", ip)
            if err == cfg.DB_ERROR:
                logger.info(f"Host IP not updated for mac = {device_mac}")
        if host["host_port"] != port:
            err = update_host(db_connection, device_mac, "host_port", port)
            if err == cfg.DB_ERROR:
                logger.info(f"Host Port not updated for mac = {device_mac}")

        # initial check-in complete now enter infinite loop while connection is active
        # all messages at this point are server driven

        while True:
            if exit_event.is_set():
                conn.close()
                return
            host_id = host["host_id"]

            # Construction requirement: get symver file and store in host_id dir if necessary
            if host["symvers_ts"] == 0:
                err = request_symver_file(conn, db_connection, host_id, device_mac)
                if err == cfg.DB_ERROR:
                    logger.info("Symver DB error occurred")
                    continue
                if err == cfg.CLOSE_SOCK:
                    conn.close()
                    break

            # Revoke requirement: get revoke list before getting report
            revoke_id_list, revoke_name_list = retrieve_revoke_list(
                db_connection, host_id
            )
            if len(revoke_id_list) > 0:
                for i in range(len(revoke_id_list)):
                    revoke_module(
                        conn,
                        db_connection,
                        host_id,
                        revoke_id_list[i],
                        revoke_name_list[i],
                    )

            # Revoke requirement: get deprecate list before getting report
            deprecate_id_list = retrieve_deprecated_list(db_connection, host_id)
            if len(deprecate_id_list) > 0:
                for i in range(len(deprecate_id_list)):
                    deprecate_module(conn, db_connection, host_id, deprecate_id_list[i])

            # Deploy requirement: toggle active_mode on/off for a deployed cust module
            toggle_active_id_list, toggle_active_mode_list = (
                retrieve_toggle_active_list(db_connection, host_id)
            )
            if len(toggle_active_id_list) > 0:
                for i in range(len(toggle_active_id_list)):
                    toggle_active(
                        conn,
                        db_connection,
                        host_id,
                        toggle_active_id_list[i],
                        toggle_active_mode_list[i],
                    )

            # Deploy requirement: set a new priority for a deployed cust module
            set_priority_id_list, set_priority_list = retrieve_set_priority_list(
                db_connection, host_id
            )
            if len(set_priority_id_list) > 0:
                for i in range(len(set_priority_id_list)):
                    set_priority(
                        conn,
                        db_connection,
                        host_id,
                        set_priority_id_list[i],
                        set_priority_list[i],
                    )

            # Monitor requirement: get a full report from host and update CIB
            logger.info(f"NCO Requesting report")
            request_report(conn, host_id)
            err = process_report(conn, db_connection, host_id, buffer_size)
            if err == cfg.CLOSE_SOCK:
                conn.close()
                break

            # Deploy requirement: need install list for host_id
            modules = retrieve_install_list(db_connection, host_id)

            logger.info(f"NCO Checking install list, {modules}")
            if len(modules) > 0:
                # send built modules to host
                send_install_modules(conn, host_id, modules)

                # update deployed table
                # TO DO: this can be handled elsewhere
                # insert_deployed(
                #    db_connection, host_id, modules, "?", "?", "?", "?", "?", "?", "?"
                # )

                # get a full report from host since we installed new modules
                logger.info(f"NCO Requesting report after install")
                request_report(conn, host_id)
                err = process_report(conn, db_connection, host_id, buffer_size)
                if err == cfg.CLOSE_SOCK:
                    conn.close()
                    break

                # TO DO: change method of removal from DB
                delete_built_module(db_connection, host_id, "IDS_web_logger")
                delete_built_module(db_connection, host_id, "DNS_response")

                # Middlebox requirement: update inverse module table (if necessary)
                update_inverse_module_requirements(db_connection, modules)

            # Security requirement: challenge deployed modules
            if args.challenge:
                # get challenge list and send challenges
                challenge_list = check_challenge(db_connection, host_id)
                for mod_id in challenge_list:
                    temp = request_challenge_response(
                        conn, db_connection, host_id, mod_id, buffer_size
                    )
                    if temp == cfg.CLOSE_SOCK:
                        conn.close()
                        break
                    if temp == cfg.REVOKE_MOD:
                        logger.info(
                            "************ Failed challenge/respnse, Revoking module************"
                        )
                        # send revoke command b/c module failed check
                        err = revoke_module(conn, db_connection, host_id, mod_id)
                        if err != 0:
                            logger.info(f"revoke module error")
                    else:
                        # update challenge ts
                        now = int(time.time())
                        update_deployed_sec_ts(db_connection, host_id, mod_id, now)

            # refresh host value each cycle in case DB updated
            host = select_host(db_connection, device_mac)

            if host == cfg.DB_ERROR:
                logger.info("Host DB error occurred, terminating connection")
                conn.close()
                break

            else:
                logger.info("Entering a wait state to interact with host again\n\n")
                start_time = int(time.time())
                end_time = start_time + host["interval"]
                interval = int(host["interval"] / 5)
                check_install_requirement_or_max_time(
                    db_connection, host_id, end_time, interval
                )
    except Exception as e:
        logger.info(f"Device thread exception: {e}")
        traceback.print_exc()
        conn.close()


def signal_handler(signum, frame):
    logger.info("SIGINT: handler called")
    exit_event.set()
    sys.exit()


if __name__ == "__main__":
    exit_event = threading.Event()
    signal.signal(signal.SIGINT, signal_handler)
    condition = threading.Condition()
    device_threads = []
    queue = multiprocessing.Queue(-1)

    try:
        # Start the NCO_UI.py process
        ui_process = subprocess.Popen(["python3", "NCO_UI.py"])
        logger.info("Started NCO_UI.py as a separate process")
    except Exception as e:
        logger.info(f"Failed to start NCO_UI.py: {e}")
        traceback.print_exc()
        sys.exit()

    try:
        # logging process starts first to allow logging for all processes
        listener = multiprocessing.Process(
            target=logger_process, args=(exit_event, queue)
        )
        listener.start()
        root_configurer(queue)

    except Exception as e:
        logger.info(f"Logging process creation error: {e}\n Terminating NCO")
        traceback.print_exc()
        sys.exit()

    try:
        # construction process runs independent of device connections
        construct = multiprocessing.Process(
            target=construction_process,
            name="constuction",
            args=(exit_event, cfg.BUILD_INTERVAL, queue),
        )
        construct.start()

    except Exception as e:
        logger.info(f"Construction process creation error: {e}")
        traceback.print_exc()

    try:
        # middlebox process runs independent of device connections
        middlebox = multiprocessing.Process(
            target=middlebox_process,
            name="middlebox",
            args=(exit_event, condition, cfg.BUILD_INTERVAL, queue),
        )
        middlebox.start()

    except Exception as e:
        logger.info(f"Middlebox process creation error: {e}")
        traceback.print_exc()

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        try:
            # timeout for listening to check if shutdown condition reached
            s.settimeout(5)
            s.bind((cfg.HOST, cfg.PORT))
            logger.info("Socket bind complete")
        except socket.error as msg:
            logger.info("Bind failed. Error : " + str(sys.exc_info()))
            sys.exit()

        s.listen()

        while True:
            if exit_event.is_set():
                break
            try:
                (conn, (ip, port)) = s.accept()
                logger.info(f"Accepting connection from {ip}:{port}")
                device = threading.Thread(
                    target=device_thread,
                    args=(conn, ip, port, cfg.MAX_BUFFER_SIZE, cfg.QUERY_INTERVAL),
                )
                device.start()
                device_threads.append(device)
            except socket.timeout:
                pass
            except Exception as e:
                logger.info(f"Device thread creation error: {e}")
                traceback.print_exc()

        s.close()
        logger.info("Joining device threads")
        for x in device_threads:
            x.join()
        exit_event.set()

    # Terminate the NCO_UI.py process when NCO.py exits
    ui_process.terminate()
    logger.info("Terminated NCO_UI.py process")
