#!/usr/bin/env python3

import socket
import os
import sys
import subprocess
import time
import json

# Used for mac address lookup
import fcntl
import struct

import platform #for linux distro

# comms with Layer 4.5
from netlink_helper import *


downloadDir = sys.argv[1]


HOST = '10.0.0.20'
PORT = 65432        # The port used by the NCO


symver_location = '/home/dan/layer4.5/'
system_name = platform.system()
system_release = platform.release()


max = 5


def send_periodic_report(conn_socket):
    send_dict = query_layer4_5()
    send_string = json.dumps(send_dict, indent=4)
    s.sendall(bytes(send_string,encoding="utf-8"))

def send_initial_report(conn_socket):
    send_dict = {}
    send_dict['mac'] =  getHwAddr('enp0s8')
    send_dict['release'] = system_release
    send_string = json.dumps(send_dict, indent=4)
    s.sendall(bytes(send_string,encoding="utf-8"))

def send_full_report(conn_socket):
    send_dict = query_layer4_5()
    send_dict['mac'] =  getHwAddr('enp0s8')
    send_dict['release'] = system_release
    send_string = json.dumps(send_dict, indent=4)
    s.sendall(bytes(send_string,encoding="utf-8"))

def send_challenge_report(conn_socket, cust_id, iv, msg):
    print(f"Challenge message {msg}")
    send_dict = challenge_layer4_5(cust_id, iv, msg)
    send_string = json.dumps(send_dict, indent=4)
    conn_socket.sendall(bytes(send_string,encoding="utf-8"))


def query_layer4_5():
    sock = Connection()

    var = 'CUST_REPORT'
    msg = Message(3,0,-1,var)

    sock.send(msg)

    report = sock.recve()

    sock.fd.close()

    payload = report.payload.decode("utf-8")
    print(f"Query Response {payload}")

    if payload == "Failed to create cust report":
        payload = "{};"

    # need to stip padded 00's from message before convert to json
    payload = payload.split(";")[0]

    return json.loads(payload)


def challenge_layer4_5(cust_id, iv, msg):
    sock = Connection()

    challenge =f'CHALLENGE {cust_id} {iv} {msg} END'
    msg = Message(3,0,-1,challenge)

    sock.send(msg)

    report = sock.recve()

    sock.fd.close()

    payload = report.payload.decode("utf-8")
    print(f"Challenge Response {payload}")

    if payload == "Failed to create cust report":
        payload = "{};"

    # need to stip padded 00's from message before convert to json
    payload = payload.split(";")[0]

    return json.loads(payload)


# each host will have a primary mac addr to report to PCC (even if multiple avail)
def getHwAddr(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    info = fcntl.ioctl(s.fileno(), 0x8927,  struct.pack('256s', bytes(ifname, 'utf-8')[:15]))
    return ':'.join('%02x' % b for b in info[18:24])


def send_symvers(conn_socket):
    filename = symver_location + 'Module.symvers'
    filesize = os.path.getsize(filename)
    send_dict = {"file" : "Module.symvers", "size":filesize}
    send_string = json.dumps(send_dict, indent=4)
    conn_socket.sendall(bytes(send_string,encoding="utf-8"))

    with open(symver_location + 'Module.symvers', 'rb') as file_to_send:
        print("symver file open")
        for data in file_to_send:
            # print("sending module")
            conn_socket.sendall(data)
        print("symvers file close")


def recv_ko_files(conn_socket, count):
    for x in range(count):
        data = conn_socket.recv(1024)
        ko_dict = json.loads(data)
        filename = ko_dict["name"]
        print(f"module name = {filename}")
        filesize = ko_dict["size"]
        print(f"module size = {filesize}")
        s.sendall(b'Clear to send')
        install_ko_file(s, filename, filesize)
        print(f"module name = {filename} completed")
    print("finished all ko modules")


def install_ko_file(conn_socket, filename, filesize):
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

    try:
        # now we need to insert the module or launch the loader service or wait for service to run?
        subprocess.run(["insmod", downloadDir+"/"+filename])

    except Exception as e:
        print(f"Exception: {e}")


# read in list of modules to remove, then finish
def retire_modules(conn_socket, count):
    for x in range(count):
        data = conn_socket.recv(1024)
        retire_dict = json.loads(data)
        filename = retire_dict["name"]
        print(f"retire module name = {filename}")

        try:
            # now we need to insert the module or launch the loader service or wait for service to run?
            subprocess.run(["rmmod", downloadDir+"/"+filename])

        except Exception as e:
            print(f"Exception: {e}")

    # # genereate new report?
    # query_dict = query_layer4_5()
    # retired = query_dict['Retired']
    # if id in retired:
    #     # success
    #     #now delete the module after we verify removed?
    #     return
    # else:
    #     # something went wrong
    #     print("error during retire module= ", filename)
    #     return


sleep_time = 10
#connect to server, send initial report and wait for server commands
# if connection terminated, then try again until server reached again
while True:
    connected = False
    stop_recv = False
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:

        while not connected:
            try:
                s.connect((HOST, PORT))
                connected=True
                send_initial_report(s)
                count = 0
            except ConnectionRefusedError:
                print("FAILED to reach server. Sleep briefly & try again")
                time.sleep(sleep_time)
                continue

        sleep_time = 30
        # basically do while loop with switch statements to perform desired action
        while not stop_recv:
            try:
                data = s.recv(1024)
                recv_dict = json.loads(data)
                print(f"Recieved message: {recv_dict}")

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
                    print(f"prepare to recv modules, count = {recv_dict['count']}")
                    s.sendall(b'Clear to send')
                    recv_ko_files(s, recv_dict["count"])


                elif recv_dict["cmd"] == "retire_module":
                    print(f"prepare to retire modules, count = {recv_dict['count']}")
                    s.sendall(b'Clear to send')
                    retire_modules(s, recv_dict["count"])


                elif recv_dict["cmd"] == "run_report":
                    send_periodic_report(s)


                elif recv_dict["cmd"] == "run_full_report":
                    send_full_report(s)


                elif recv_dict["cmd"] == "challenge":
                    send_challenge_report(s, recv_dict["id"], recv_dict["iv"], recv_dict["msg"])

            except Exception as e:
                print(f"Command parsing error, {e}")