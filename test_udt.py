#!/usr/bin/env python

import sys
import socket
import unittest

import udt
import _udt

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
    
    def test_socket_fileno(self):
        s = self.create_socket()
        self.assert_(isinstance(s.fileno(), int))

    def test_getset_sockopt_mss(self):
        s = self.create_socket()
        val = s.getsockopt(0, udt.UDT_MSS)
        self.assertEquals(val, 1500)

        s.setsockopt(0, udt.UDT_MSS, 1800)
        val = s.getsockopt(0, udt.UDT_MSS)
        self.assertEquals(val, 1800)

    def test_getset_sockopt_sndsyn(self):
        s = self.create_socket()
        val = s.getsockopt(0, udt.UDT_SNDSYN)
        self.assertEquals(val, True)

        s.setsockopt(0, udt.UDT_SNDSYN, False)
        val = s.getsockopt(0, udt.UDT_SNDSYN)
        self.assertEquals(val, False)

    def test_getset_sockopt_rcvsyn(self):
        s = self.create_socket()
        val = s.getsockopt(0, udt.UDT_RCVSYN)
        self.assertEquals(val, True)

    def test_getset_sockopt_fc(self):
        s = self.create_socket()
        val = s.getsockopt(0, udt.UDT_FC)
        self.assertEquals(val, 25600)

        s.setsockopt(0, udt.UDT_FC, 10000)
        val = s.getsockopt(0, udt.UDT_FC)
        self.assertEquals(val, 10000)

    def test_getset_sockopt_sndbuf(self):
        s = self.create_socket()
        val = s.getsockopt(0, udt.UDT_SNDBUF)
        self.assertEquals(val, 12058624)

        s.setsockopt(0, udt.UDT_SNDBUF, 198720)
        val = s.getsockopt(0, udt.UDT_SNDBUF)
        self.assertEquals(val, 198720)

    def test_getsockopt_rcvbuf(self):
        s = self.create_socket()
        val = s.getsockopt(0, udt.UDT_RCVBUF)
        self.assertEquals(val, 12058624)

        s.setsockopt(0, udt.UDT_RCVBUF, 198720)
        val = s.getsockopt(0, udt.UDT_RCVBUF)
        self.assertEquals(val, 198720)

    def test_getsockopt_udp_sndbuf(self):
        s = self.create_socket()
        val = s.getsockopt(0, udt.UDP_SNDBUF)

        s.setsockopt(0, udt.UDP_SNDBUF, 10000)
        val = s.getsockopt(0, udt.UDP_SNDBUF)
        self.assertEquals(val, 10000)

    def test_getsockopt_udp_rcvbuf(self):
        s = self.create_socket()
        val = s.getsockopt(0, udt.UDP_RCVBUF)

    def test_getsockopt_snd_timeout(self):
        s = self.create_socket()
        val = s.getsockopt(0, udt.UDT_SNDTIMEO)
        self.assertEquals(val, -1)

    def test_getsockopt_rcv_timeout(self):
        s = self.create_socket()
        val = s.getsockopt(0, udt.UDT_RCVTIMEO)
        self.assertEquals(val, -1)

    def test_getsockopt_reuseaddr(self):
        s = self.create_socket()
        val = s.getsockopt(0, udt.UDT_REUSEADDR)
        self.assertEquals(val, True)

    def test_getsockopt_linger(self):
        s = self.create_socket()
        val = s.getsockopt(0, udt.UDT_LINGER)
        self.assertEquals(val, (1, 180))

    def test_getsockopt_max_bw(self):
        s = self.create_socket()
        val = s.getsockopt(0, udt.UDT_MAXBW)
        self.assertEquals(val, -1)

    def test_create_epoll(self):
        epoll = udt.epoll()
        self.assert_(epoll.eid)

    def test_epoll_release(self):
        epoll = udt.epoll()
        epoll.release()

    def test_epoll_double_release(self):
        epoll = udt.epoll()
        epoll.release()
        self.assertRaises(RuntimeError, epoll.release)

    def test_epoll_add_usock(self):
        epoll = udt.epoll()
        s  = self.create_socket()
        self.assertEquals(0, epoll.add_usock(s.fileno(), udt.UDT_EPOLL_IN))

    def test_epoll_add_ssock(self):
        epoll = udt.epoll()
        s1, s2 = socket.socketpair()
        epoll.add_ssock(s1.fileno(), udt.UDT_EPOLL_IN)

    def test_epoll_remove_usock(self):
        epoll = udt.epoll()
        s  = self.create_socket()
        epoll.add_usock(s.fileno(), udt.UDT_EPOLL_IN)
        epoll.remove_usock(s.fileno(), udt.UDT_EPOLL_IN)

    def test_epoll_remove_bad_usock(self):
        epoll = udt.epoll()
        s  = self.create_socket()
        fileno = s.fileno()
        s.close()
        self.assertRaises(RuntimeError, epoll.remove_usock, fileno, udt.UDT_EPOLL_IN)

    # FIXME - broken functionality in UDT ?
    def _test_epoll_remove_ssock(self):
        epoll = udt.epoll()
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(('127.0.0.1', 22))
        self.assertEquals(
            epoll.add_ssock(s.fileno(), udt.UDT_EPOLL_IN),
            0
        )
        epoll.remove_ssock(s.fileno(), udt.UDT_EPOLL_IN)

    def test_epoll_wait(self):
        s = self.create_socket()
        epoll = udt.epoll()
        epoll.add_usock(s.fileno(), udt.UDT_EPOLL_IN)
        print epoll.epoll_wait(0)
        print epoll.epoll_wait(1)

unittest.main()

