#!/usr/bin/env python3

import socket
import argparse


parser = argparse.ArgumentParser(description='Echo server')
parser.add_argument('--proto', type=str, required=True, help="tcp or udp")
parser.add_argument('--ip', type=str, required=False, help="Server IP")
parser.add_argument('--port', type=int, required=False, help="Server port")

args = parser.parse_args()


#default values
HOST = '127.0.0.1'  # Standard loopback interface address (localhost)
PORT = 65432        # Port to listen on (non-privileged ports are > 1023)


if args.ip:
    HOST=args.ip

if args.port:
    PORT=args.port


if args.proto == "tcp":
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, PORT))
        s.listen()
        print('\nTCP server waiting to receive connection')
        conn, addr = s.accept()
        with conn:
            print(f"Connected by {addr}")
            while True:
                data = conn.recv(1024)
                if not data:
                    break
                print(f"recv: {data}")
                conn.sendall(data)


if args.proto == "udp":
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.bind((HOST, PORT))
        while True:
            print('\nUDP server waiting to receive message')
            data, address = s.recvfrom(1024)
            if not data:
                break
            print(f"received {data} from {address}")
            sent = s.sendto(data, address)
