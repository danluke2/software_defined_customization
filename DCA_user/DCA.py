#!/usr/bin/env python3

import socket
import os
import subprocess
import time
import json
import argparse
# Used for mac address lookup
import fcntl
import struct

# import platform #for linux distro
import cfg

# comms with Layer 4.5
from netlink_helper import *

# remove prints and log instead
import logging
from logging import handlers


parser = argparse.ArgumentParser(description='DCA user space program')
parser.add_argument('--ip', type=str, required=False, help="NCO IP")
parser.add_argument('--port', type=int, required=False, help="NCO port")
parser.add_argument('--dir', type=str, required=False,
                    help="KO Module download dir")
parser.add_argument('--iface', type=str, required=False,
                    help="Interface name for MAC")
parser.add_argument(
    '--controlled', help="Require user input to start checkin process", action="store_true")
parser.add_argument(
    '--print', help="Enables logging to console", action="store_true")
parser.add_argument('--logfile', type=str, required=False,
                    help="Full log file path to use, defaults to layer4_5 directory")

args = parser.parse_args()


if args.ip:
    cfg.HOST = args.ip

if args.port:
    cfg.PORT = args.port

if args.iface:
    cfg.INTERFACE = args.iface

if args.dir:
    cfg.download_dir = args.dir

if args.print:
    cfg.log_console = True

if args.logfile:
    cfg.log_file = args.logfile


logger = logging.getLogger(__name__)  # use module name


def logger_configurer():
    root = logging.getLogger()
    file_handler = handlers.RotatingFileHandler(
        cfg.log_file, 'a', cfg.log_size, cfg.log_max)
    file_formatter = logging.Formatter('%(asctime)s  %(message)s')
    file_handler.setFormatter(file_formatter)
    root.addHandler(file_handler)
    root.setLevel(logging.DEBUG)

    if cfg.log_console:
        console_handler = logging.StreamHandler()
        console_formatter = logging.Formatter('%(message)s')
        console_handler.setFormatter(console_formatter)
        root.addHandler(console_handler)


def send_periodic_report(conn_socket):
    message = 'CUST_REPORT'
    send_dict = query_layer4_5(message)
    send_string = json.dumps(send_dict, indent=4)
    # logging.info(f"Periodic report: {send_string}")
    conn_socket.sendall(bytes(send_string, encoding="utf-8"))


def send_initial_report(conn_socket):
    send_dict = {}
    send_dict['mac'] = getHwAddr(cfg.INTERFACE)
    send_dict['release'] = cfg.system_release
    send_string = json.dumps(send_dict, indent=4)
    logging.info(f"Initial report: {send_string}")
    conn_socket.sendall(bytes(send_string, encoding="utf-8"))


def send_full_report(conn_socket):
    message = 'CUST_REPORT'
    send_dict = query_layer4_5(message)
    send_dict['mac'] = getHwAddr(cfg.INTERFACE)
    send_dict['release'] = cfg.system_release
    send_string = json.dumps(send_dict, indent=4)
    logging.info(f"Full report: {send_string}")
    conn_socket.sendall(bytes(send_string, encoding="utf-8"))


def send_challenge_report(conn_socket, cust_id, iv, msg):
    logging.info(f"Challenge message {msg}")
    challenge = f'CHALLENGE {cust_id} {iv} {msg} END'
    send_dict = query_layer4_5(challenge)
    send_string = json.dumps(send_dict, indent=4)
    conn_socket.sendall(bytes(send_string, encoding="utf-8"))


def send_deprecate_report(conn_socket, cust_id):
    logging.info(f"Deactive cust id {cust_id}")
    deprecate = f'DEPRECATE {cust_id} END'
    send_dict = query_layer4_5(deprecate)
    send_string = json.dumps(send_dict, indent=4)
    conn_socket.sendall(bytes(send_string, encoding="utf-8"))


def send_toggle_report(conn_socket, cust_id, mode):
    logging.info(f"Toggle cust id {cust_id} active mode to {mode}")
    toggle = f'TOGGLE {cust_id} {mode} END'
    send_dict = query_layer4_5(toggle)
    send_string = json.dumps(send_dict, indent=4)
    conn_socket.sendall(bytes(send_string, encoding="utf-8"))


def query_layer4_5(message):
    sock = Connection()

    msg = Message(3, 0, -1, message)

    sock.send(msg)

    report = sock.recve()

    sock.fd.close()

    payload = report.payload.decode("utf-8")

    if payload == "Failed to create cust report":
        payload = "{};"

    # need to stip padded 00's from message before convert to json
    payload = payload.split(";")[0]

    logging.info(f"Query Response {payload}")

    return json.loads(payload)


# each host will have a primary mac addr to report to PCC (even if multiple avail)


def getHwAddr(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    info = fcntl.ioctl(s.fileno(), 0x8927,  struct.pack(
        '256s', bytes(ifname, 'utf-8')[:15]))
    return ':'.join('%02x' % b for b in info[18:24])


def send_symvers(conn_socket):
    filename = cfg.symver_location + 'Module.symvers'
    filesize = os.path.getsize(filename)
    send_dict = {"file": "Module.symvers", "size": filesize}
    send_string = json.dumps(send_dict, indent=4)
    conn_socket.sendall(bytes(send_string, encoding="utf-8"))

    data = conn_socket.recv(cfg.MAX_BUFFER_SIZE)
    data = data.decode("utf-8")
    if data != 'Clear to send':
        logging.info("NCO can't accept")
        return

    with open(cfg.symver_location + 'Module.symvers', 'rb') as file_to_send:
        # logging.info("symver file open")
        for data in file_to_send:
            # logging.info("sending module")
            conn_socket.sendall(data)
        logging.info("symvers file sent")


def recv_ko_files(conn_socket, count):
    for x in range(count):
        data = conn_socket.recv(cfg.MAX_BUFFER_SIZE)
        ko_dict = json.loads(data)
        filename = ko_dict["name"]
        filesize = ko_dict["size"]
        logging.info(f"module name = {filename}, size = {filesize}")
        conn_socket.sendall(b'Clear to send')
        install_ko_file(conn_socket, filename, filesize)
        logging.info(f"module name = {filename} completed")
    logging.info("finished all ko modules")


def install_ko_file(conn_socket, filename, filesize):
    with open(os.path.join(cfg.download_dir, filename), 'wb') as file_to_write:
        while True:
            data = conn_socket.recv(filesize)
            if not data:
                break
            file_to_write.write(data)
            filesize -= len(data)
            # logging.info("remaining = ", filesize)
            if filesize <= 0:
                break
        file_to_write.close()

    try:
        # now we need to insert the module or launch the loader service or wait for service to run?
        subprocess.run(["insmod", cfg.download_dir+"/"+filename])

    except Exception as e:
        logging.info(f"Exception: {e}")


# read in list of modules to remove, then finish
def revoke_module(conn_socket, filename):
    full_path = cfg.download_dir + "/" + filename + ".ko"
    logging.info(f"revoke module = {full_path}")
    result = 0
    try:
        # now we need to remove the module
        subprocess.run(["rmmod", filename], check=True)
        # now remove the module so it won't be loaded on reboot
        subprocess.run(["rm", full_path], check=True)
        conn_socket.sendall(b'success')
    except subprocess.CalledProcessError as e:
        result = -1
        temp = (f"{e}").encode('utf-8')
        conn_socket.sendall(temp)
        logging.info(f"Exception: {e}")
    return result


logger_configurer()

# connect to server, send initial report and wait for server commands
# if connection terminated, then try again until server reached again
# while (input("DCA loop again?") == "y"):
while True:
    connected = False
    stop_recv = False
    log_print = True
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:

        while not connected:
            try:
                s.connect((cfg.HOST, cfg.PORT))
                connected = True
                send_initial_report(s)
                count = 0
            except ConnectionRefusedError:
                if log_print:
                    logging.info(
                        "FAILED to reach server. Sleep briefly & try again loop")
                    log_print = False
                time.sleep(cfg.nco_connect_sleep_time)
                continue

        # basically do while loop with switch statements to perform desired action
        while not stop_recv:
            try:
                data = s.recv(cfg.MAX_BUFFER_SIZE)
                recv_dict = json.loads(data)
                logging.info(f"Recieved message: {recv_dict}")

            except json.decoder.JSONDecodeError as e:
                count += 1
                if count >= cfg.max_errors:
                    logging.info("max json errors reached")
                    stop_recv = True

            except OSError as e:
                logging.info(f"OSError {e}")
                stop_recv = True

            except Exception as e:
                logging.info(f"Error during data reception: {e}")
                count += 1
                if count >= cfg.max_errors:
                    logging.info("max general errors reached")
                    stop_recv = True

            try:
                if recv_dict["cmd"] == "send_symvers":
                    logging.info("requested module.symvers file")
                    send_symvers(s)

                elif recv_dict["cmd"] == "recv_module":
                    logging.info(
                        f"prepare to recv modules, count = {recv_dict['count']}")
                    s.sendall(b'Clear to send')
                    recv_ko_files(s, recv_dict["count"])

                elif recv_dict["cmd"] == "revoke_module":
                    logging.info(f"revoke request received")
                    revoke_module(s, recv_dict["name"])

                elif recv_dict["cmd"] == "deprecate_module":
                    logging.info(f"deprecate request")
                    send_deprecate_report(s, recv_dict["id"])

                elif recv_dict["cmd"] == "toggle_active":
                    logging.info(f"toggle active request")
                    send_toggle_report(
                        s, recv_dict["id"], recv_dict["mode"])

                elif recv_dict["cmd"] == "run_report":
                    logging.info(f"report request received")
                    send_periodic_report(s)

                elif recv_dict["cmd"] == "run_full_report":
                    logging.info(f"full report request")
                    send_full_report(s)

                elif recv_dict["cmd"] == "challenge":
                    logging.info(f"challenge request")
                    send_challenge_report(
                        s, recv_dict["id"], recv_dict["iv"], recv_dict["msg"])

            except Exception as e:
                logging.info(f"Command parsing error, {e}")
                logging.info(f"Command received was: {recv_dict}")
                count += 1
                if count >= cfg.max_errors:
                    logging.info(f"Max exceptions recv, breaking connection")
                    break
