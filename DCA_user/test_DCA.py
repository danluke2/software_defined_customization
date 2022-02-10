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

import platform #for linux distro
import argparse

# comms with Layer 4.5
from netlink_helper import *



HOST = '10.0.0.20'
PORT = 65432        # The port used by the NCO


symver_location = '/home/dan/layer4.5/'


sleep_time = 10


# read in argument values to pretend to be multiple different hosts talking
# with the server for testing distribution limits


parser = argparse.ArgumentParser(description='Process some integers.')
parser.add_argument('--start', type=int, required=True)
parser.add_argument('--end', type=int, required=True)
parser.add_argument('--dir', type=str, required=True)


args = parser.parse_args()


max = 5


def send_periodic_report(conn_socket, report_dict, host_id):
    send_string = json.dumps(report_dict, indent=4)
    conn_socket.sendall(bytes(send_string,encoding="utf-8"))
    print(f"Host {host_id} periodic report")

def send_initial_report(conn_socket, host_id):
    system_name = platform.system()
    system_release = platform.release()
    send_dict = {}
    send_dict['mac'] =  host_id
    send_dict['release'] = system_release
    send_string = json.dumps(send_dict, indent=4)
    conn_socket.sendall(bytes(send_string,encoding="utf-8"))
    print(f"Host {host_id} initial report")
    report_dict = {"Active":[], "Retired":[]}
    return report_dict

# def send_full_report(conn_socket, host_id, report_dict):
#     send_dict = report_dict.copy()
#     send_dict['mac'] =  "host_" + host_id
#     send_dict['release'] = system_release
#     send_string = json.dumps(send_dict, indent=4)
#     s.sendall(bytes(send_string,encoding="utf-8"))
#     print(send_string)


# simulated
# def query_layer4_5():
#     sock = Connection()
#
#     var = 'update'
#     msg = Message(3,0,-1,var)
#
#     sock.send(msg)
#
#     report = sock.recve()
#
#     sock.fd.close()
#
#     payload = report.payload.decode("utf-8")
#     print(payload)
#
#     if payload == "Failed to create cust report":
#         payload = "{};"
#
#     # need to stip padded 00's from message before convert to json
#     payload = payload.split(";")[0]
#
#     return json.loads(payload)


# faking MAC addr
# each host will have a primary mac addr to report to PCC (even if multiple avail)
# def getHwAddr(ifname):
#     s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
#     info = fcntl.ioctl(s.fileno(), 0x8927,  struct.pack('256s', bytes(ifname, 'utf-8')[:15]))
#     return ':'.join('%02x' % b for b in info[18:24])


#this is fine since file will exist
def send_symvers(conn_socket):
    filename = symver_location + 'Module.symvers'
    filesize = os.path.getsize(filename)
    send_dict = {"file" : "Module.symvers", "size":filesize}
    send_string = json.dumps(send_dict, indent=4)
    conn_socket.sendall(bytes(send_string,encoding="utf-8"))

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
        # print(f"module name = {filename}")
        filesize = ko_dict["size"]
        # print(f"module size = {filesize}")
        conn_socket.sendall(b'Clear to send')
        install_ko_file(conn_socket, filename, filesize, downloadDir, report_dict, host_id)
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
            # print("remaining = ", filesize)
            if filesize<=0:
                break
        file_to_write.close()

    # don't install, instead update report_dict as if installed
    # try:
    #     # now we need to insert the module or launch the loader service or wait for service to run?
    #     subprocess.run(["insmod", downloadDir+"/"+filename])
    #
    # except Exception as e:
    #     print(f"Exception: {e}")
    report_dict["Active"] = [{"ID":int(host_id),"Count":int(host_id),"ts":int(host_id)}]


# read in list of modules to remove, then finish
def retire_modules(conn_socket, count, report_dict, host_id):
    for x in range(count):
        data = conn_socket.recv(1024)
        retire_dict = json.loads(data)
        filename = retire_dict["name"]
        print(f"retire module name = {filename}")
        #pretend to remove module
        # try:
        #     # now we need to insert the module or launch the loader service or wait for service to run?
        #     subprocess.run(["rmmod", downloadDir+"/"+filename])
        #
        # except Exception as e:
        #     print(f"Exception: {e}")
        report_dict["Active"] = []
        report_dict["Retired"] = [{"ID":int(host_id),"ts":int(host_id)}]



def fake_host_thread(host_id, downloadDir):
    sleep_time = 10
    max = 5
    #connect to server, send initial report and wait for server commands
    # if connection terminated, then try again until server reached again
    connected = False
    stop_recv = False
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:

        while not connected:
            try:
                s.connect((HOST, PORT))
                connected=True
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
                count +=1
                if count >= max:
                    print("max json errors reached")
                    stop_recv = True

            except OSError as e:
                print(f"OSError {e}")
                stop_recv = True

            except Exception as e:
                print(f"Error during data reception: {e}")
                count +=1
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
                    recv_ko_files(s, recv_dict["count"], downloadDir, report_dict, host_id)


                elif recv_dict["cmd"] == "retire_module":
                    # print(f"prepare to retire modules, count = {recv_dict['count']}")
                    s.sendall(b'Clear to send')
                    retire_modules(s, recv_dict["count"], report_dict, host_id)


                elif recv_dict["cmd"] == "run_report":
                    send_periodic_report(s, report_dict, host_id)

                # elif recv_dict["cmd"] == "run_full_report":
                #     send_full_report(s, host_id, report_dict)

            except Exception as e:
                print(f"Command parsing error, {e}")


for x in range(args.start, args.end):
    try:
        host_id = str(x+1)
        #build module thread runs independent of client connections
        # passing a tuple so host_id is not split into one char at a time as an arg
        Thread(target=fake_host_thread, args=(host_id,args.dir)).start();
        print(f"Fake host {host_id} thread running")
        time.sleep(0.5)
    except:
        print("Client thread creation error!")
        traceback.print_exc()
