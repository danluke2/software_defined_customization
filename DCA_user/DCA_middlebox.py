#!/usr/bin/env python3

# Middlebox DCA: assumes no Layer 4.5 capability

import socket
import os
import sys
import subprocess
import time
import json
import argparse
# Used for mac address lookup
import fcntl
import struct

# import platform #for linux distro
import cfg
# remove prints and log instead
import logging


parser = argparse.ArgumentParser(description='DCA middlebox user space program')
parser.add_argument('--ip', type=str, required=False, help="NCO IP")
parser.add_argument('--port', type=int, required=False, help="NCO middlebox port")
parser.add_argument('--dir', type=str, required=False, help="Inverse Module download dir")
parser.add_argument('--type', type=str, required=False, help="Middlebox Type Info")
parser.add_argument('--iface', type=str, required=False, help="Interface name for MAC")
parser.add_argument('--logging', help="Enable logging to file instead of print to console", action="store_true" )
parser.add_argument('--logfile', type=str, required=False, help="Full log file path to use, defaults to layer4_5 directory")

args = parser.parse_args()



if args.ip:
    cfg.HOST=args.ip

if args.port:
    cfg.MIDDLE_PORT=args.port

if args.iface:
    cfg.INTERFACE=args.iface

if args.dir:
    cfg.middle_dir = args.dir

if args.type:
    cfg.middle_type = args.type

if args.logging:
    if args.logfile:
        logging.basicConfig(format='%(levelname)s:%(message)s', filename=args.logfile, level=logging.DEBUG)
    else:
        logging.basicConfig(format='%(levelname)s:%(message)s', filename=cfg.log_file, level=logging.DEBUG)
else:
    logging.basicConfig(format='%(levelname)s:%(message)s', stream=sys.stdout, level=logging.DEBUG)






def send_initial_report(conn_socket):
    send_dict = {}
    send_dict['mac'] =  getHwAddr(cfg.INTERFACE)
    send_dict['release'] = cfg.system_release
    send_dict['type']=cfg.middle_type
    send_string = json.dumps(send_dict, indent=4)
    conn_socket.sendall(bytes(send_string,encoding="utf-8"))


# def send_periodic_report(conn_socket):
#     send_dict = inverse_dir_lookup()
#     send_string = json.dumps(send_dict, indent=4)
#     conn_socket.sendall(bytes(send_string,encoding="utf-8"))

# def inverse_dir_lookup():
#     # get a list of all installed inverse modules
#
#     return json.loads(payload)



# each host will have a primary mac addr to report to PCC (even if multiple avail)
def getHwAddr(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    info = fcntl.ioctl(s.fileno(), 0x8927,  struct.pack('256s', bytes(ifname, 'utf-8')[:15]))
    return ':'.join('%02x' % b for b in info[18:24])



def recv_inverse_module(conn_socket, filename, filesize):
    conn_socket.sendall(b'Clear to send')
    with open(os.path.join(cfg.middle_dir, filename), 'wb') as file_to_write:
        while True:
            data = conn_socket.recv(filesize)
            if not data:
                break
            file_to_write.write(data)
            filesize -= len(data)
            # logging.info("remaining = ", filesize)
            if filesize<=0:
                break
        file_to_write.close()
    logging.info(f"Inverse module name = {filename} completed")



# read in list of modules to remove, then finish
def revoke_inverse_module(conn_socket, filename):
    logging.info(f"revoke module = {cfg.middle_dir}/{filename}")
    result = 0
    try:
        # now we need to remove the module
        subprocess.run(["rm", cfg.middle_dir + '/' + filename], check=True)
        conn_socket.sendall(b'success')
    except subprocess.CalledProcessError as e:
        result = -1
        temp = (f"{e}").encode('utf-8')
        conn_socket.sendall(temp)
        logging.info(f"Exception: {e}")
    return result




#connect to server, send initial report and wait for server commands
# if connection terminated, then try again until server reached again
# while (input("DCA middlebox loop again?") == "y"):
while True:
    connected = False
    stop_recv = False
    log_print = True
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:

        while not connected:
            try:
                s.connect((cfg.HOST, cfg.MIDDLE_PORT))
                connected=True
                send_initial_report(s)
                count = 0
                logging.info(f"Connected to NCO at {cfg.HOST}:{cfg.MIDDLE_PORT}")
            except ConnectionRefusedError:
                if log_print:
                    logging.info("FAILED to reach server. Sleep briefly & try again loop")
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
                count +=1
                if count >= cfg.max_errors:
                    logging.info("max json errors reached")
                    stop_recv = True

            except OSError as e:
                logging.info(f"OSError {e}")
                stop_recv = True

            except Exception as e:
                logging.info(f"Error during data reception: {e}")
                count +=1
                if count >= cfg.max_errors:
                    logging.info("max general errors reached")
                    stop_recv = True

            try:
                if recv_dict["cmd"] == "recv_inverse":
                    logging.info(f"prepare to recv inverse module, name = {recv_dict['name']}")
                    recv_inverse_module(s, recv_dict['name'], recv_dict['size'])


                elif recv_dict["cmd"] == "revoke_inverse_module":
                    revoke_inverse_module(s, recv_dict["name"])


                # elif recv_dict["cmd"] == "run_report":
                #     send_periodic_report(s)


            except Exception as e:
                logging.info(f"Command parsing error, {e}")
                break
