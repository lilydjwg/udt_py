#!/usr/bin/env python

import socket
import os
import time
import threading
import rpyc

import dspyte.core

class Pinger(threading.Thread):
    def run(self):
        for i in range(60):
            print "xxxxxx", i, time.time()
            time.sleep(1)
            
class ConnectionTest(dspyte.core.Main):
    """
    Basic connection test
    """
    name = 'ConnectionTest'
    def __call__(self, agents, *args, **kwargs):
        server = agents['server']
        client = agents['client']
        server.agent.listen()
        #p = Pinger()
        #p.start()
        client.agent.connect()
        server.agent.accept()
        client.agent.send_data()
        server.agent.recv_data()

class StreamServer(dspyte.core.Agent):
    port = 7778
    name = 'server'
    def listen(self):
        import udt
        self.s = udt.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
        self.send_msg('binding')
        self.s.bind(('localhost', self.port))
        self.send_msg("bound to port: %s" % self.port)
        self.s.listen(10)

    def accept(self):
        import udt
        self.client, addr  = self.s.accept()
        self.send_msg('accepted client!')

    def recv_data(self):
        self.send_msg("got %s" % self.client.recv(1024, 0))
        
class StreamClient(dspyte.core.Agent):
    name = 'client'
    def connect(self):
        import udt
        import ctypes
        self.s = udt.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
        print self.s.connect(("localhost", StreamServer.port))
        self.send_msg("connection ok!")

    def send_data(self):
        self.s.send("Hello World!", 100)
