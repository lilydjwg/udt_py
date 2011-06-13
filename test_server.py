#!/usr/bin/env python

import udt
import socket
import time
import threading

class Pinger(threading.Thread):
    def run(self):
        for i in range(300):
            print "ping ...", i
            time.sleep(1)

p = Pinger()
p.start()

s = udt.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
epoll = udt.epoll()
s.bind(("127.0.0.1", 5555))
s.listen(10)


while 1:
    client, addr = s.accept()
    print "accept", client, addr
    epoll.add_usock(client.fileno(), udt.UDT_EPOLL_IN)
    print 'wait..'
    print epoll.epoll_wait(-1)
    print 'got data..'
    client.send("Hello World" * 10, -1)
    client.close()
