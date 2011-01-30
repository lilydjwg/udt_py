#!/usr/bin/python

import udt
import socket
import time

s = udt.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
s.bind(("127.0.0.1", 5555))
s.listen(10)
while 1:
    client = s.accept()
    for i in range(100):
        print client.perfmon(1)
        client.sendmsg("Hello World" * 10, -1, 0)
    client.close()
    
