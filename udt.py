#!/usr/bin/python

# use socket library for portability
import socket as socketlib
from _udt import *
import _udt

class socket(_udt.socket):
    def connect(self, addr):
        conn_addr = self._get_addr(addr)
        return _udt.socket.connect(self, conn_addr)

    def bind(self, addr):
        bind_addr = self._get_addr(addr)
        return _udt.socket.bind(self, bind_addr)

    def _get_addr(self, (host, port)):
        family, socktype, proto, name, addr = socketlib.getaddrinfo(
            host, 
            port, 
            self.family,
            0,
            self.proto,
            0
        )[0]
        return addr
