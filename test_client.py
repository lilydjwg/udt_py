#!/usr/bin/env python3

import udt
import socket
import time

s = udt.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
s.connect(("localhost", 5555))

print("Sending...")
s.send(b"Hello", 0)
buf = s.recv(1024, 0)
print(repr(buf))
