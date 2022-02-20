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

import platform #for linux distro



parser = argparse.ArgumentParser(description='DCA middlebox user space program')
parser.add_argument('--ip', type=str, required=False, help="NCO IP")
parser.add_argument('--port', type=int, required=False, help="NCO middlebox port")
parser.add_argument('--dir', type=str, required=False, help="Inverse Module download dir")
parser.add_argument('--type', type=str, required=False, help="Middlebox Type Info")
parser.add_argument('--iface', type=str, required=False, help="Interface name for MAC")

args = parser.parse_args()


HOST = '10.0.0.20'
PORT = 65433        # The middlebox port used by the NCO
INTERFACE="enp0s3"
type="Wireshark"
system_name = platform.system()
system_release = platform.release()

# assuming Wireshark plugin dir
download_dir = "/usr/lib/x86_64-linux-gnu/wireshark/plugins"

max = 5

if args.ip:
    HOST=args.ip

if args.port:
    PORT=args.port

if args.iface:
    INTERFACE=args.iface

if args.dir:
    download_dir = args.dir

if args.type:
    type = args.type





def send_periodic_report(conn_socket):
    send_dict = inverse_dir_lookup()
    send_string = json.dumps(send_dict, indent=4)
    conn_socket.sendall(bytes(send_string,encoding="utf-8"))

def send_initial_report(conn_socket):
    send_dict = {}
    send_dict['mac'] =  getHwAddr(INTERFACE)
    send_dict['release'] = system_release
    send_dict['type']=type
    send_string = json.dumps(send_dict, indent=4)
    conn_socket.sendall(bytes(send_string,encoding="utf-8"))



def inverse_dir_lookup():
    # get a list of all installed inverse modules

    return json.loads(payload)



# each host will have a primary mac addr to report to PCC (even if multiple avail)
def getHwAddr(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    info = fcntl.ioctl(s.fileno(), 0x8927,  struct.pack('256s', bytes(ifname, 'utf-8')[:15]))
    return ':'.join('%02x' % b for b in info[18:24])



def recv_inverse_module(conn_socket, filename, filesize):
    conn_socket.sendall(b'Clear to send')
    with open(os.path.join(download_dir, filename), 'wb') as file_to_write:
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
    print(f"Inverse module name = {filename} completed")



# read in list of modules to remove, then finish
def revoke_inverse_module(conn_socket, filename):
    print(f"revoke module = {download_dir}/{filename}")
    result = 0
    try:
        # now we need to remove the module
        subprocess.run(["rm", download_dir + '/' + filename], check=True)
        conn_socket.sendall(b'success')
    except subprocess.CalledProcessError as e:
        result = -1
        temp = (f"{e}").encode('utf-8')
        conn_socket.sendall(temp)
        print(f"Exception: {e}")
    return result




sleep_time = 10
#connect to server, send initial report and wait for server commands
# if connection terminated, then try again until server reached again
while (input("DCA middlebox loop again?") == "y"):
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
                if recv_dict["cmd"] == "recv_inverse":
                    print(f"prepare to recv inverse module, name = {recv_dict['name']}")
                    recv_inverse_module(s, recv_dict['name'], recv_dict['size'])


                elif recv_dict["cmd"] == "revoke_inverse_module":
                    revoke_inverse_module(s, recv_dict["name"])


                elif recv_dict["cmd"] == "run_report":
                    send_periodic_report(s)


            except Exception as e:
                print(f"Command parsing error, {e}")
                break
