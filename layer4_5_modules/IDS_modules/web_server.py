import http.server
import socketserver
import logging

# Set the address and port
PORT = 8080


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

# Set up the server
with socketserver.TCPServer(("", PORT), RequestHandler) as httpd:
    logging.info(f"Serving HTTP on port {PORT}...")
    httpd.serve_forever()
