#!/usr/bin/env python3
"""
Very simple HTTP server in python for logging requests
Usage::
    ./server.py [<port>]
"""
from http.server import BaseHTTPRequestHandler, HTTPServer
from http.server import SimpleHTTPRequestHandler
import logging
import os
import re
import sys
import io
from http import HTTPStatus
import uuid

base_dir = os.getcwd()


def sanitize_filename(filename: str) -> str:
    """
    Replaces all forbidden chars with '' and removes unnecessary whitespaces
    If, after sanitization, the given filename is empty, the function will return 'file_[UUID][ext]'
    :param filename: filename to be sanitized
    :return: sanitized filename
    """
    chars = ['\\', '/', ':', '*', '?', '"', '<', '>', '|']

    filename = filename.translate({ord(x): '' for x in chars}).strip()
    name = re.sub(r'\.[^.]+$', '', filename)
    extension = re.search(r'(\.[^.]+$)', filename)
    extension = extension.group(1) if extension else ''

    return filename if name else f'file_{uuid.uuid4().hex}{extension}'


class S(SimpleHTTPRequestHandler):

    def do_POST(self):
        """Serve a POST request."""
        # upload file
        result, message = self.handle_upload()

        r = []
        enc = sys.getfilesystemencoding()

        # html code of upload result page
        r.append('<!DOCTYPE HTML>')
        r.append('<html>\n<title>Upload result</title>')
        r.append('<body>\n<h1>Upload result</h1>')
        if result:
            r.append(
                '<b><font color="green">File(s) successfully uploaded</font></b>: ')
            r.append(f'{", ".join(message)}.')
        else:
            r.append('<b><font color="red">Failed to upload file(s)</font></b>: ')
            r.append(message)
        r.append(
            f'<br /><br />\n<a href=\"{self.headers["referer"]}\">Go back</a>')
        r.append('</body>\n</html>')

        encoded = '\n'.join(r).encode(enc, 'surrogateescape')
        f = io.BytesIO()
        f.write(encoded)
        f.seek(0)

        self.send_response(HTTPStatus.OK)
        self.send_header('Content-type', 'text/html')
        self.send_header('Content-Length', str(len(encoded)))
        self.end_headers()

        if f:
            self.copyfile(f, self.wfile)
            f.close()

    def handle_upload(self):
        """Handle the file upload."""

        # extract boundary from headers
        boundary = re.search(
            f'boundary=([^;]+)', self.headers['content-type']).group(1)

        # read all bytes (headers included)
        # 'readlines()' hangs the script because it needs the EOF character to stop,
        # even if you specify how many bytes to read
        # 'file.read(nbytes).splitlines(True)' does the trick because 'read()' reads 'nbytes' bytes
        # and 'splitlines(True)' splits the file into lines and retains the newline character
        data = self.rfile.read(
            int(self.headers['content-length'])).splitlines(True)

        # find all filenames
        filenames = re.findall(f'{boundary}.+?filename="(.+?)"', str(data))

        if not filenames:
            return False, 'couldn\'t find file name(s).'

        filenames = [sanitize_filename(filename) for filename in filenames]

        # find all boundary occurrences in data
        boundary_indices = list((i for i, line in enumerate(
            data) if re.search(boundary, str(line))))

        # save file(s)
        for i in range(len(filenames)):
            # remove file headers
            file_data = data[(boundary_indices[i] + 4):boundary_indices[i+1]]

            # join list of bytes into bytestring
            file_data = b''.join(file_data)

            # write to file
            try:
                with open(f'{base_dir}/{filenames[i]}', 'wb') as file:
                    file.write(file_data)
            except IOError:
                return False, f'couldn\'t save {filenames[i]}.'

        return True, filenames

    def do_PUT(self):
        path = self.translate_path(self.path)
        if path.endswith('/'):
            self.send_response(405, "Method Not Allowed")
            self.wfile.write("PUT not allowed on a directory\n".encode())
            return
        else:
            try:
                os.makedirs(os.path.dirname(path))
            except FileExistsError:
                pass
            length = int(self.headers['Content-Length'])
            with open(path, 'wb') as f:
                f.write(self.rfile.read(length))
            self.send_response(201, "Created")
            self.send_header('Content-type', 'text/html')
            self.end_headers()


def run(server_class=HTTPServer, handler_class=S, port=8080):
    logging.basicConfig(level=logging.INFO)
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    logging.info('Starting httpd...\n')
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.server_close()
    logging.info('Stopping httpd...\n')


if __name__ == '__main__':
    from sys import argv

    if len(argv) == 2:
        run(port=int(argv[1]))
    else:
        run()
