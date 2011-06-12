#!/usr/bin/env python

import udt
import socket
import time

s = udt.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
s.connect(("localhost", 5555))
while(1):
    time.sleep(10)
    print "Sending..."
    s.send("Hello", 0)
    buf = s.recv(1024, 0)
    print `buf`
