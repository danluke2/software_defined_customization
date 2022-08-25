# taken from http://www.piware.de/2011/01/creating-an-https-server-in-python/
# generate server.pem with the following command:
#    openssl req -new -x509 -keyout key.pem -out server.pem -days 365 -nodes
# run as follows:
#    python simple-https-server.py
# then in your browser, visit:
#    https://localhost:4443


import http.server
import ssl

# ************** STANDARD PARAMS MUST GO HERE ****************
HOST='10.0.0.20'
SIMPLE_SERVER_DIR='/home/vagrant/software_defined_customization/experiment_scripts/client_server'
# ************** END STANDARD PARAMS ****************

cert_location = SIMPLE_SERVER_DIR + "/server.pem"
key_location = SIMPLE_SERVER_DIR + "/key.pem"

server_address = (HOST, 443)
httpd = http.server.HTTPServer(
    server_address, http.server.SimpleHTTPRequestHandler)
httpd.socket = ssl.wrap_socket(httpd.socket,
                               server_side=True,
                               certfile=cert_location,
                               keyfile=key_location,
                               ssl_version=ssl.PROTOCOL_TLS)
httpd.serve_forever()
