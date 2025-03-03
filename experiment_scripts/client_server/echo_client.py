#!/usr/bin/env python3

import socket
import argparse
import os

parser = argparse.ArgumentParser(description="Echo client")
parser.add_argument("--tcp", help="TCP connection", action="store_true")
parser.add_argument("--udp", help="UDP connection", action="store_true")
parser.add_argument("--sip", type=str, required=False, help="Server IP")
parser.add_argument("--sport", type=int, required=False, help="Server port")
parser.add_argument("--cip", type=str, required=False, help="Client IP")
parser.add_argument("--cport", type=int, required=False, help="Client port")

args = parser.parse_args()


# default values
SERVER_IP = "127.0.0.1"  # Standard loopback interface address (localhost)
SERVER_PORT = 65432  # Port to listen on (non-privileged ports are > 1023)

CLIENT_IP = "10.0.0.30"
CLIENT_PORT = 65234

if args.sip:
    SERVER_IP = args.sip

if args.sport:
    SERVER_PORT = args.sport

"""if args.cip:
    CLIENT_IP = args.cip

if args.cport:
    CLIENT_PORT = args.cport"""

pid = os.getpid()


if args.tcp:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        print(f"\nTCP client connecting to server, PID={pid}")
        # s.bind((CLIENT_IP, CLIENT_PORT))
        s.connect((SERVER_IP, SERVER_PORT))
        s.settimeout(10)
        s.sendall(b"Hello")
        data = s.recv(1024)
        print(f"Received {data}")
        while True:
            test = input("type here\n")
            s.sendall(test.encode())
            if test == "quit":
                break
            try:
                data = s.recv(1024)
            except:
                continue
            print(f"Received {data}")

    print(f"Received {data}")
    print("Exiting")


elif args.udp:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        print(f"\nUDP client connecting to server, PID={pid}")
        s.bind((CLIENT_IP, CLIENT_PORT))
        s.settimeout(10)
        # s.sendto(b"Hello", (CLIENT_IP, CLIENT_PORT))
        s.sendto(b"Hello", (SERVER_IP, SERVER_PORT))
        data, server = s.recvfrom(1024)
        print(f"Received {data}")
        while True:
            test = input("type here\n")
            # s.sendto(test.encode(), (SERVER_IP, SERVER_PORT))
            s.sendto(test.encode(), (CLIENT_IP, CLIENT_PORT))
            if test == "quit":
                break
            try:
                data, server = s.recvfrom(1024)
            except:
                continue
            print(f"Received {data}")

    print(f"Received {data}")
    print("Exiting")
