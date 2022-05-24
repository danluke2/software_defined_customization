#!/usr/bin/env python3

import socket
import os
import sys
import subprocess
import time
import json

from threading import Thread
import traceback

# Used for mac address lookup
import fcntl
import struct

import platform  # for linux distro
import argparse


# read in argument values to pretend to be multiple different hosts talking
# with the server for testing distribution limits
parser = argparse.ArgumentParser(description='Process some integers.')
parser.add_argument('--number', type=int, required=True,
                    help="Number of devices to emulate")
parser.add_argument('--dir', type=str, required=True,
                    help="KO Module download dir")
parser.add_argument('--ip', type=str, required=False, help="NCO IP")
parser.add_argument('--port', type=int, required=False, help="NCO port")

args = parser.parse_args()


HOST = '10.0.0.20'
PORT = 65432        # The port used by the NCO

if args.ip:
    HOST = args.ip

if args.port:
    PORT = args.port


system_name = platform.system()
system_release = platform.release()
symver_location = '/usr/lib/modules/' + system_release + '/layer4_5/'


sleep_time = 5
max = 5


def send_periodic_report(conn_socket, report_dict, host_id):
    send_string = json.dumps(report_dict, indent=4)
    conn_socket.sendall(bytes(send_string, encoding="utf-8"))
    print(f"Host {host_id} periodic report")


def send_initial_report(conn_socket, host_id):
    send_dict = {}
    # simulating mac address with host id for easy matching/tracking
    send_dict['mac'] = host_id
    send_dict['release'] = system_release
    send_string = json.dumps(send_dict, indent=4)
    conn_socket.sendall(bytes(send_string, encoding="utf-8"))
    print(f"Host {host_id} initial report")
    # simulating installed/revoked tracking since emulating multiple hosts
    report_dict = {"Installed": [], "Revoked": []}
    return report_dict


# this is fine since file will exist
def send_symvers(conn_socket):
    filename = symver_location + 'Module.symvers'
    filesize = os.path.getsize(filename)
    send_dict = {"file": "Module.symvers", "size": filesize}
    send_string = json.dumps(send_dict, indent=4)
    conn_socket.sendall(bytes(send_string, encoding="utf-8"))

    data = conn_socket.recv(1024)
    data = data.decode("utf-8")
    if data != 'Clear to send':
        print("NCO can't accept")
        return

    with open(symver_location + 'Module.symvers', 'rb') as file_to_send:
        # print("symver file open")
        for data in file_to_send:
            # print("sending module")
            conn_socket.sendall(data)
        # print("symvers file close")


# we simulate installing the file by just updating our report
def recv_ko_files(conn_socket, count, downloadDir, report_dict, host_id):
    for x in range(count):
        data = conn_socket.recv(1024)
        ko_dict = json.loads(data)
        filename = "host_" + host_id + "_test.ko"
        filesize = ko_dict["size"]
        conn_socket.sendall(b'Clear to send')
        install_ko_file(conn_socket, filename, filesize,
                        downloadDir, report_dict, host_id)
        print(f"module name = {filename} completed")
    # print("finished all ko modules")


def install_ko_file(conn_socket, filename, filesize, downloadDir, report_dict, host_id):
    with open(os.path.join(downloadDir, filename), 'wb') as file_to_write:
        while True:
            data = conn_socket.recv(filesize)
            if not data:
                break
            file_to_write.write(data)
            filesize -= len(data)
            if filesize <= 0:
                break
        file_to_write.close()

    # don't install, instead update report_dict as if installed
    report_dict["Installed"] = [
        {"ID": int(host_id), "Count": int(host_id), "ts": int(host_id)}]


def emulated_host_thread(host_id, downloadDir):
    sleep_time = 5
    max = 5
    # connect to server, send initial report and wait for server commands
    # if connection terminated, then test is over
    connected = False
    stop_recv = False
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:

        while not connected:
            try:
                s.connect((HOST, PORT))
                connected = True
                report_dict = send_initial_report(s, host_id)
                count = 0
            except ConnectionRefusedError:
                print("FAILED to reach server. Sleep briefly & try again")
                time.sleep(sleep_time)
                continue

        # basically do while loop with switch statements to perform desired action
        while not stop_recv:
            try:
                data = s.recv(1024)
                recv_dict = json.loads(data)

            except json.decoder.JSONDecodeError as e:
                count += 1
                if count >= max:
                    print("max json errors reached")
                    stop_recv = True

            except OSError as e:
                print(f"OSError {e}")
                stop_recv = True

            except Exception as e:
                print(f"Error during data reception: {e}")
                count += 1
                if count >= max:
                    print("max general errors reached")
                    stop_recv = True

            try:
                if recv_dict["cmd"] == "send_symvers":
                    print("requested module.symvers file")
                    send_symvers(s)

                elif recv_dict["cmd"] == "recv_module":
                    # print(f"prepare to recv modules, count = {recv_dict['count']}")
                    s.sendall(b'Clear to send')
                    recv_ko_files(
                        s, recv_dict["count"], downloadDir, report_dict, host_id)

                elif recv_dict["cmd"] == "run_report":
                    send_periodic_report(s, report_dict, host_id)

                # # command just for finishing test
                # elif recv_dict["cmd"] == "close":
                #     s.close()
                #     return

            except Exception as e:
                print(f"Command parsing error, {e}")


print(f"Starting {args.number} device threads")
for x in range(args.number):
    try:
        host_id = str(x+1)

        # passing a tuple so host_id is not split into one char at a time as an arg
        Thread(target=emulated_host_thread, args=(host_id, args.dir)).start()
        print(f"Emulated host {host_id} thread running")
        time.sleep(0.5)
    except:
        print("Host thread creation error!")
        traceback.print_exc()
