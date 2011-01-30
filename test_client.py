#!/usr/bin/python

import udt
import socket
import time

s = udt.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
s.connect(("localhost", 5555))
while(1):
    buf = s.recvmsg(1024)
    print `buf`
    print

