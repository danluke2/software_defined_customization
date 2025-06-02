import http.server
import socketserver
import logging
import ssl
import os

# Set the ports for HTTP and HTTPS
HTTP_PORT = 80
HTTPS_PORT = 443


class RequestHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        # Log the requested path
        logging.info(f"Received GET request: {self.path}")

        # Get the 'Host' header
        host = self.headers.get("Host")

        # Check if 'exampleurl.com' is in the Host header
        if host and "exampleurl.com" in host:
            logging.info("ALERT: exampleurl.com detected in the Host header!")

        # Call the parent class to handle the rest (sending response)
        super().do_GET()


# Set up logging for requests
logging.basicConfig(level=logging.INFO)

# Get the directory of the current Python file
current_dir = os.path.dirname(os.path.abspath(__file__))

# Paths to the key and certificate files
keyfile_path = os.path.join(
    current_dir, "private.key"
)  # Replace with your key file name
certfile_path = os.path.join(
    current_dir, "certificate.crt"
)  # Replace with your certificate file name


# Start the HTTP server
def start_http_server():
    with socketserver.TCPServer(("", HTTP_PORT), RequestHandler) as httpd:
        logging.info(f"Serving HTTP on port {HTTP_PORT}...")
        httpd.serve_forever()


# Start the HTTPS server
def start_https_server():
    with socketserver.TCPServer(("", HTTPS_PORT), RequestHandler) as httpd:
        # Wrap the server socket with SSL
        httpd.socket = ssl.wrap_socket(
            httpd.socket,
            keyfile=keyfile_path,  # Use the dynamically constructed key file path
            certfile=certfile_path,  # Use the dynamically constructed certificate file path
            server_side=True,
        )
        logging.info(f"Serving HTTPS on port {HTTPS_PORT}...")
        httpd.serve_forever()


# Run both servers
if __name__ == "__main__":
    import threading

    # Start HTTP server in a separate thread
    http_thread = threading.Thread(target=start_http_server, daemon=True)
    http_thread.start()

    # Start HTTPS server in the main thread
    start_https_server()
