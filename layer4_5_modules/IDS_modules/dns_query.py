import socket

# DNS Server (Use 8.8.8.8 for Google's Public DNS, or your custom DNS server)
DNS_SERVER = "8.8.8.8"
DNS_PORT = 53
TARGET_DOMAIN = "example.com"

# DNS Query Packet for "example.com"
DNS_QUERY = b"\xaa\xaa"  # Transaction ID (randomized)
DNS_QUERY += b"\x01\x00"  # Standard query
DNS_QUERY += b"\x00\x01"  # Questions: 1
DNS_QUERY += b"\x00\x00"  # Answer RRs: 0
DNS_QUERY += b"\x00\x00"  # Authority RRs: 0
DNS_QUERY += b"\x00\x00"  # Additional RRs: 0

# Convert domain to DNS query format (e.g., "example.com" -> 7example3com0)
for part in TARGET_DOMAIN.split("."):
    DNS_QUERY += bytes([len(part)]) + part.encode()
DNS_QUERY += b"\x00"  # End of domain
DNS_QUERY += b"\x00\x01"  # Query type A (IPv4 address)
DNS_QUERY += b"\x00\x01"  # Query class IN (Internet)

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print(f"Sending 10 DNS queries to {DNS_SERVER}...")

# Send exactly 10 DNS queries
for i in range(10):
    try:
        sock.sendto(DNS_QUERY, (DNS_SERVER, DNS_PORT))
        print(f"Sent DNS request {i + 1}/10")
    except Exception as e:
        print(f"Error sending DNS query: {e}")
        break

sock.close()
print("Completed sending 10 DNS queries.")
