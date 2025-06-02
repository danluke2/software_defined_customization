import socket
import threading
import time
import csv
from collections import defaultdict

# DNS Server Settings
DNS_PORT = 53
DNS_IP = "10.0.8.21"
LOG_INTERVAL = 1  # Log query counts every 1 second
CSV_FILENAME = "dns_query_log.csv"

# Dictionary to store DNS query counts per IP per interval
dns_query_counts = defaultdict(int)  # Tracks queries per interval
all_ips = set()  # Tracks all IPs that have ever connected
lock = threading.Lock()


def dns_server():
    """Listens for DNS queries and tracks counts."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((DNS_IP, DNS_PORT))
    print(f"[*] DNS Server Running on {DNS_IP}:{DNS_PORT}")

    try:
        while True:
            data, addr = sock.recvfrom(512)
            ip = addr[0]

            # Ignore local loopback requests
            if ip == "127.0.0.1":
                continue

            with lock:
                dns_query_counts[ip] += 1  # Count only within the interval
                all_ips.add(ip)  # Track all IPs that have ever connected
                print(f"[+] Received DNS Query from {addr}")
    except Exception as e:
        print(f"[!] Error in DNS server: {e}")
    finally:
        sock.close()


def log_query_counts():
    """Logs the DNS query counts per interval to a CSV file."""
    with open(CSV_FILENAME, "w", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(["Elapsed Time (s)", "IP Address", "Queries"])

    start_time = time.time()

    while True:
        time.sleep(LOG_INTERVAL)
        elapsed_time = int(time.time() - start_time)

        with lock:
            if dns_query_counts or all_ips:
                with open(CSV_FILENAME, "a", newline="") as csvfile:
                    writer = csv.writer(csvfile)
                    for ip in all_ips:
                        count = dns_query_counts.get(ip, 0)
                        writer.writerow([elapsed_time, ip, count])

                print(f"Logged {len(all_ips)} IP(s) at {elapsed_time}s")

            # Reset the count for the next interval
            dns_query_counts.clear()


if __name__ == "__main__":
    try:
        threading.Thread(target=dns_server, daemon=True).start()
        threading.Thread(target=log_query_counts, daemon=True).start()

        while True:
            time.sleep(1)  # Keep the main thread alive
    except KeyboardInterrupt:
        print("\n[!] Server shutting down...")
