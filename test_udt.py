#!/usr/bin/python

import unittest
import udt
import _udt
import socket

class TestSocket(unittest.TestCase):
    def create_socket(self):
        return udt.socket(socket.AF_INET, socket.SOCK_STREAM, 0)

    def create_int_socket(self):
        return _udt.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
    
    def test_init(self):
        s = self.create_socket()    
        self.assertEquals(s.family, socket.AF_INET)
        self.assertEquals(s.type,   socket.SOCK_STREAM)
        self.assertEquals(s.proto, 0)

    def test_not_enough_args_init(self):    
        self.assertRaises(TypeError, udt.socket, ())

    def test_close(self):   
        s = self.create_socket()
        # perhaps this should fail since it was never open?
        s.close()

    def test_double_close(self):    
        s = self.create_socket()
        s.close()
        self.assertRaises(RuntimeError, s.close, ())

    def test_connect_bad_args(self):    
        addr = ("192.168.0.1", 2222)
        s = self.create_int_socket()
        # 0 args
        self.assertRaises(TypeError, s.connect, ())
        # 1 arg
        self.assertRaises(TypeError, s.connect, "localhost", 22, )
        # string port
        self.assertRaises(TypeError, s.connect, ("localhost", "22"))

    def test_connect_no_listen(self):      
        s = self.create_socket()
        self.assertRaises(RuntimeError, s.connect, ("127.0.0.1", 2344))
        self.assertRaises(RuntimeError, s.connect, ("localhost", 2344))

    def test_bind_ok(self):      
        s = self.create_socket()
        s.bind(("127.0.0.1", 3333))

    def test_startup(self):
        udt.startup()
    
    def test_cleanup(self):
        udt.cleanup()

unittest.main()

