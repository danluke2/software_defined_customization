#!/usr/bin/env python3

import socket
import argparse
import os


parser = argparse.ArgumentParser(description="Echo client")
parser.add_argument("--tcp", help="TCP connection", action="store_true")
parser.add_argument("--udp", help="UDP connection", action="store_true")
parser.add_argument("--sip", type=str, required=False, help="Server IP")
parser.add_argument("--sport", type=int, required=False, help="Server port")

args = parser.parse_args()


# default values
SERVER_IP = "127.0.0.1"  # Standard loopback interface address (localhost)
SERVER_PORT = 65432  # Port to listen on (non-privileged ports are > 1023)


if args.sip:
    SERVER_IP = args.sip

if args.sport:
    SERVER_PORT = args.sport


pid = os.getpid()


if args.tcp:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((SERVER_IP, SERVER_PORT))
        s.listen()
        print(f"\nTCP server waiting to receive connection, PID={pid}")
        conn, addr = s.accept()
        with conn:
            print(f"Connected by {addr}")
            while True:
                data = conn.recv(1024)
                if not data:
                    break
                print(f"recv: {data}")
                conn.sendall(data)


if args.udp:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.bind((SERVER_IP, SERVER_PORT))
        while True:
            print(f"\nUDP server waiting to receive message, PID={pid}")
            data, address = s.recvfrom(1024)
            if not data:
                break
            print(f"received {data} from {address}")
            sent = s.sendto(data, address)
            if data == b"quit":
                break
