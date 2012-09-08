#!/usr/bin/env python3
# vim:fileencoding=utf-8

import sys
import udt
import struct
import socket

try:
  fn = sys.argv[3].encode()
  server = sys.argv[1]
  port = int(sys.argv[2])
except (IndexError, ValueError):
  sys.exit('usage: <server_addr> <server_port> <file_name>')

# udt.startup()
s = udt.socket(socket.AF_INET, socket.SOCK_STREAM, socket.AI_PASSIVE)
s.connect((server, port))

l = struct.pack('I', len(fn))
s.send(l, 0)
s.send(fn, 0)
a = s.recv(8, 0)
print(a)
l = struct.unpack('q', a)[0]
print('Size:', l)
if l < 0:
  sys.exit('File not found')

left = l
with open(fn, 'wb') as f:
  while left > 0:
    data = s.recv(l, 0)
    f.write(data)
    left -= len(data)
