#!/usr/bin/env python3

import socket
import argparse


parser = argparse.ArgumentParser(description='Echo client')
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
        s.connect((HOST, PORT))
        s.settimeout(10)
        s.sendall(b'Hello')
        data = s.recv(1024)
        print(f"Received {data}")
        while True:
            test = input("type here\n")
            s.sendall(test.encode())
            if test == 'quit':
                break
            try:
                data = s.recv(1024)
            except:
                continue
            print(f"Received {data}")

    print(f"Received {data}")
    print('Exiting')


if args.proto == "udp":
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.settimeout(10)
        s.sendto(b'Hello', (HOST, PORT))
        data, server = s.recvfrom(1024)
        print(f"Received {data}")
        while True:
            test = input("type here\n")
            s.sendto(test.encode(), (HOST, PORT) )
            if test == 'quit':
                break
            try:
                data, server = s.recvfrom(1024)
            except:
                continue
            print(f"Received {data}")

    print(f"Received {data}")
    print('Exiting')
