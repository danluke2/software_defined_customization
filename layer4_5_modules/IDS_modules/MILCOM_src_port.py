# Simple python code to send data on source port >1024

import socket
import threading
import time

SOURCE_PORT = 1050
DEST_PORT = 80
HOST = "127.0.0.1"
DATA = b"Hello from source port >1024"


def server():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, DEST_PORT))
        s.listen(1)
        conn, addr = s.accept()
        with conn:
            print(f"[Server] Connection from {addr}")
            received = conn.recv(1024)
            print(f"[Server] Received: {received.decode()}")


def client():
    time.sleep(0.5)  # Small delay to ensure server is listening
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, SOURCE_PORT))  # Explicit source port >1024
        s.connect((HOST, DEST_PORT))
        print(f"[Client] Connected from port {SOURCE_PORT} to port {DEST_PORT}")
        s.sendall(DATA)
        print("[Client] Data sent.")


if __name__ == "__main__":
    threading.Thread(target=server, daemon=True).start()
    client()
