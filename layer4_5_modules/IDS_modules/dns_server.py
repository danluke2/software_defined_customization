import socket

DNS_PORT = 53  # Listen on port 5000
DNS_IP = "127.0.0.53"  # Listen on all interfaces


def dns_server():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((DNS_IP, DNS_PORT))
    print(f"[*] DNS Server Running on {DNS_IP}:{DNS_PORT}")

    while True:
        data, addr = sock.recvfrom(512)  # Receive query
        print(f"[+] Received DNS Query from {addr}")
        response = (
            data[:2]
            + b"\x81\x80"
            + data[4:6]
            + data[4:6]
            + b"\x00\x00\x00\x00"
            + data[12:]
            + b"\xc0\x0c\x00\x01\x00\x01\x00\x00\x00\x3c\x00\x04\x7f\x00\x00\x01"
        )
        sock.sendto(response, addr)  # Send fake response


dns_server()
