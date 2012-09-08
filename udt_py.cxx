#include <Python.h>
#include <iostream>
#include <memory>
#include <arpa/inet.h>
#include <set>

#include "udt_py.h"

PyObject* pyudt_socket_accept(PyObject *self, PyObject *args, PyObject *kwargs);

static PyObject* pyudt_epoll_add_usock(PyObject *self, PyObject *args, PyObject *kwargs);
static PyObject* pyudt_epoll_remove_usock(PyObject *self, PyObject *args, PyObject *kwargs);

static PyObject* pyudt_epoll_add_ssock(PyObject *self, PyObject *args, PyObject *kwargs);
static PyObject* pyudt_epoll_remove_ssock(PyObject *self, PyObject *args, PyObject *kwargs);

static PyObject* pyudt_epoll_wait(PyObject *self, PyObject *args, PyObject *kwargs);

#define PY_TRY_CXX \
try \
{

#define PY_CATCH_CXX(ret_val) \
} \
catch(py_udt_error& e) \
{ \
    try \
    { \
        UDT::ERRORINFO& udt_err = UDT::getlasterror(); \
        PyErr_SetString(PyExc_RuntimeError, udt_err.getErrorMessage()); \
        UDT::getlasterror().clear(); \
        return ret_val; \
    } \
    catch(...)\
    {\
        PyErr_SetString(PyExc_RuntimeError, "UDT error"); \
        UDT::getlasterror().clear(); \
        return ret_val; \
    }\
} \
catch(std::exception& e) \
{ \
    PyErr_SetString(PyExc_RuntimeError, e.what()); \
    return ret_val; \
} \
catch(...) \
{\
    PyErr_SetString(PyExc_RuntimeError, "C++ error"); \
    return ret_val; \
}


AutoDecref::AutoDecref(PyObject *ptr)
{
    this->ptr = ptr;   
}

AutoDecref::~AutoDecref()
{
    if(this->ptr != NULL)
    {
        Py_DECREF(ptr);
    }
}

void AutoDecref::ok()
{
    this->ptr = NULL;
}

AutoGILCallOut::AutoGILCallOut()
{
    state = PyEval_SaveThread();
}

AutoGILCallOut::~AutoGILCallOut()
{
    PyEval_RestoreThread(state);
}

AutoGILCallBack::AutoGILCallBack()
{
    state = PyGILState_Ensure();
}

AutoGILCallBack::~AutoGILCallBack()
{
     PyGILState_Release(state);
}

PyObject* pyudt_cleanup(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    if(!PyArg_ParseTuple(args, ""))
    {
        return NULL;
    }

    if (UDT::cleanup() == UDT::ERROR)
    {
        throw py_udt_error();
    }

    Py_RETURN_NONE;

PY_CATCH_CXX(NULL)
}

PyObject* pyudt_startup(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    if(!PyArg_ParseTuple(args, ""))
    {
        return NULL;
    }

    if (UDT::startup() == UDT::ERROR)
    {
        throw py_udt_error();
    }

    Py_RETURN_NONE;

PY_CATCH_CXX(NULL)
}

RecvBuffer::RecvBuffer(unsigned int len)
{
    head = new char[len + 1];
    buf_len = 0;
    max_buf_len = len;
    head[max_buf_len] = '\0';
}

RecvBuffer::~RecvBuffer()
{
    delete[] head;
}

char *RecvBuffer::get_head()
{
    return head;
}

unsigned int RecvBuffer::get_max_len()
{
    return max_buf_len;
}

unsigned int RecvBuffer::get_buf_len()
{
    return buf_len;
}

unsigned int RecvBuffer::set_buf_len(unsigned int new_len)
{
    /* FIXME - overflow check required */
    buf_len = new_len;
    head[buf_len] = '\0';
    return buf_len;
}

int pyudt_epoll_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    if(!PyArg_ParseTuple(args, ""))
    {
        return -1;
    }

    pyudt_epoll_object* py_epoll = ((pyudt_epoll_object*)self);
    py_epoll->eid = UDT::epoll_create();
        
    if(py_epoll->eid < 0)
    {
        throw py_udt_error();
    }    
    return 0;
PY_CATCH_CXX(-1)
}

PyObject* pyudt_epoll_release(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    if(!PyArg_ParseTuple(args, ""))
    {
        return NULL;
    }

    if(UDT::epoll_release(((pyudt_epoll_object*)self)->eid))
    {
        throw py_udt_error();
    }
    Py_RETURN_NONE;
PY_CATCH_CXX(NULL)
}

static PyObject* pyudt_epoll_get_eid(PyObject *py_epoll)
{
    return Py_BuildValue("i", ((pyudt_epoll_object*)py_epoll)->eid);
}

static PyGetSetDef pyudt_epoll_getset[] = {
    {
        (char*)"eid", 
        (getter)pyudt_epoll_get_eid, 
        NULL,
        (char*)"get epoll id",
        NULL
    },
    {NULL}  /* Sentinel */
};

static PyMethodDef pyudt_epoll_methods[] = {
    {
        "release",  
        (PyCFunction)pyudt_epoll_release, 
        METH_VARARGS, 
        "epoll release"
    },
    {
        "add_usock",  
        (PyCFunction)pyudt_epoll_add_usock, 
        METH_VARARGS, 
        "add udt socket to epoll"
    },
    {
        "add_ssock",  
        (PyCFunction)pyudt_epoll_add_ssock, 
        METH_VARARGS, 
        "add system socket to epoll"
    },
    {
        "remove_usock",  
        (PyCFunction)pyudt_epoll_remove_usock, 
        METH_VARARGS, 
        "remove udt socket from epoll"
    },
    {
        "remove_ssock",  
        (PyCFunction)pyudt_epoll_remove_ssock, 
        METH_VARARGS, 
        "remove system socket from epoll"
    },
    {
        "epoll_wait",  
        (PyCFunction)pyudt_epoll_wait, 
        METH_VARARGS, 
        "wait on a epoll events"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject pyudt_epoll_type = {
    PyObject_HEAD_INIT(NULL)
    "_udt.socket",                  /* tp_name */
    sizeof(pyudt_socket_object),    /* tp_basicsize */
    0,                              /* tp_itemsize */
    0,                              /* tp_dealloc */
    0,                              /* tp_print */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_reserved */
    0,                              /* tp_repr */
    0,                              /* tp_as_number */
    0,                              /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash */
    0,                              /* tp_call */
    0,                              /* tp_str */
    0,                              /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,/* tp_flags */
    "UDT epoll",                   /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    0,                              /* tp_iter */
    0,                              /* tp_iternext */
    pyudt_epoll_methods,            /* tp_methods */
    0,                              /* tp_members */
    pyudt_epoll_getset,             /* tp_getset */
    0,                              /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    (initproc)pyudt_epoll_init,     /* tp_init */
    0,                              /* tp_alloc */
    0,                              /* tp_new */

};


static PyObject* pyudt_socket_get_family(PyObject *py_socket)
{
    return Py_BuildValue("i", ((pyudt_socket_object*)py_socket)->family);
}

static PyObject* pyudt_socket_get_type(PyObject *py_socket)
{
    return Py_BuildValue("i", ((pyudt_socket_object*)py_socket)->type);
}

static PyObject* pyudt_socket_get_proto(PyObject *py_socket)
{
    return Py_BuildValue("i", ((pyudt_socket_object*)py_socket)->proto);
}
PyObject* pyudt_socket_connect(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX
    char *address;
    int port = 0;
    
    sockaddr_in serv_addr;

    pyudt_socket_object* py_socket  = ((pyudt_socket_object*)self);
    if(!PyArg_ParseTuple(args, "(si)", &address, &port))
    {
        return NULL;
    }

    serv_addr.sin_family = py_socket->family;
    serv_addr.sin_port = htons(port);
    int res = inet_pton(py_socket->family, address, &serv_addr.sin_addr);

    if(res == 0)
    {
        PyErr_SetString(PyExc_ValueError, "bad address");
        return NULL;
    }
    else if(res == -1 && errno == EAFNOSUPPORT)
    {
        PyErr_SetString(PyExc_ValueError, "address family not supported");
        return NULL;
    }

    memset(&(serv_addr.sin_zero), '\0', 8);

    if (UDT::ERROR == UDT::connect(
            py_socket->cc_socket, 
            (sockaddr*)&serv_addr, sizeof(serv_addr)))
    {
        throw py_udt_error();
    }
    Py_RETURN_NONE;

PY_CATCH_CXX(NULL)
}


PyObject* pyudt_socket_bind(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX
    char *address;
    int port;
    
    sockaddr_in serv_addr;

    pyudt_socket_object* py_socket  = ((pyudt_socket_object*)self);
    if(!PyArg_ParseTuple(args, "(si)", &address, &port))
    {
        return NULL;
    }

    int res = inet_pton(py_socket->family, address, &serv_addr.sin_addr);

    serv_addr.sin_family = py_socket->family;
    serv_addr.sin_port = htons(port);

    if(res == 0)
    {
        PyErr_SetString(PyExc_ValueError, "bad address");
        return NULL;
    }
    else if(res == -1 && errno == EAFNOSUPPORT)
    {
        PyErr_SetString(PyExc_ValueError, "address family not supported");
        return NULL;
    }

    memset(&(serv_addr.sin_zero), '\0', 8);
        
    if (UDT::ERROR == UDT::bind(
            py_socket->cc_socket, 
            (sockaddr*)&serv_addr, sizeof(serv_addr)))
    {
        throw py_udt_error();
    }
    Py_RETURN_NONE;

PY_CATCH_CXX(NULL)
}

PyObject* pyudt_socket_close(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    if(UDT::close(((pyudt_socket_object*)self)->cc_socket))
    {
        throw py_udt_error();
    }
    Py_RETURN_NONE;
PY_CATCH_CXX(NULL)
}

PyObject* pyudt_socket_listen(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX
    int backlog;
    if(!PyArg_ParseTuple(args, "i", &backlog))
    {
        return NULL;
    }

    if(UDT::listen(((pyudt_socket_object*)self)->cc_socket, backlog))
    {
        throw py_udt_error();
    }
    Py_RETURN_NONE;

PY_CATCH_CXX(NULL)
}
    
int pyudt_socket_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    int family;
    int type; 
    int proto;

    if(!PyArg_ParseTuple(args, "iii", &family, &type, &proto))
    {
        return -1;
    }

    if(family != AF_INET)
    {
        PyErr_SetString(PyExc_ValueError, "UDT only supports AF_INET addresses");
        return -1;
    }

    UDTSOCKET cc_socket = UDT::socket(family, type, proto);
    if(cc_socket == -1)
    {
        PyErr_SetString(PyExc_RuntimeError, "socket creation returned -1");
        return -1;
    }

    pyudt_socket_object* py_socket = ((pyudt_socket_object*)self);
    py_socket->cc_socket = cc_socket;
    py_socket->family = family;
    py_socket->type = type;
    py_socket->proto = proto;

    return 0;
PY_CATCH_CXX(-1)
}

PyObject* pyudt_socket_send(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    char* buf;
    int   buf_len;
    int   flags;
    int   send_len = 0;

    pyudt_socket_object* py_socket = ((pyudt_socket_object*)self);

    if(!PyArg_ParseTuple(args, "y#i", &buf, &buf_len, &flags))
    {
        return NULL;
    }
    
    {
        AutoGILCallOut g;
        send_len = UDT::send(py_socket->cc_socket, buf, buf_len, flags);
    }

    if(send_len == UDT::ERROR) 
    {
        throw py_udt_error();
    }
    return Py_BuildValue("i", send_len);

PY_CATCH_CXX(NULL)
}


PyObject* pyudt_socket_sendmsg(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    char* buf;
    int   buf_len;
    int   ttl;
    int   send_len = 0;
    int   in_order = 0;

    pyudt_socket_object* py_socket = ((pyudt_socket_object*)self);

    if(!PyArg_ParseTuple(args, "y#ii", &buf, &buf_len, &ttl, &in_order))
    {
        return NULL;
    }
    
    send_len = UDT::sendmsg(py_socket->cc_socket, buf, buf_len, ttl, in_order);

    if(send_len == UDT::ERROR) 
    {
        throw py_udt_error();
    }
    return Py_BuildValue("i", send_len);

PY_CATCH_CXX(NULL)
}

PyObject* pyudt_socket_recv(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    int len;
    int flags;
    int recv_len = 0;

    pyudt_socket_object* py_socket = ((pyudt_socket_object*)self);

    if(!PyArg_ParseTuple(args, "ii", &len, &flags))
    {
        return NULL;
    }
    
    RecvBuffer recv_buf(len);
    recv_len = UDT::recv(py_socket->cc_socket, recv_buf.get_head(), len, flags);

    if(recv_len == UDT::ERROR) 
    {
        throw py_udt_error();
    }

    recv_buf.set_buf_len(recv_len);
    return Py_BuildValue("y#", recv_buf.get_head(), recv_len);

PY_CATCH_CXX(NULL)
}

PyObject* pyudt_socket_recvmsg(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    int len;
    int recv_len = 0;

    pyudt_socket_object* py_socket = ((pyudt_socket_object*)self);

    if(!PyArg_ParseTuple(args, "i", &len))
    {
        return NULL;
    }
    
    RecvBuffer recv_buf(len);
    recv_len = UDT::recvmsg(py_socket->cc_socket, recv_buf.get_head(), len);

    if(recv_len == UDT::ERROR) 
    {
        throw py_udt_error();
    }

    recv_buf.set_buf_len(recv_len);
    return Py_BuildValue("y#", recv_buf.get_head(), recv_len);

PY_CATCH_CXX(NULL)
}

PyObject* pyudt_socket_perfmon(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX
    int clear = 0;
    pyudt_socket_object* py_socket = ((pyudt_socket_object*)self);

    if(!PyArg_ParseTuple(args, "i", &clear))
    {
        return NULL;
    }

    UDT::TRACEINFO info;
    if(UDT::perfmon(py_socket->cc_socket, &info, clear) == UDT::ERROR)
    {
        throw py_udt_error();
    }
    /* Slow */   
    return Py_BuildValue(
        "{sLsLsLsisisisisisisisLsLsisisisisisisisdsdsdsisisisdsdsisi}",
        /* Aggregate Values */
        "msTimeStamp",   
        info.msTimeStamp,   
        "pktSentTotal",
        info.pktSentTotal,
        "pktRecvTotal",
        info.pktRecvTotal,
        "pktSndLossTotal",
        info.pktSndLossTotal,
        "pktRcvLossTotal",
        info.pktRcvLossTotal,
        "pktRetransTotal",
        info.pktRetransTotal,
        "pktSentACKTotal",
        info.pktSentACKTotal,
        "pktRecvACKTotal",
        info.pktRecvACKTotal,
        "pktSentNAKTotal",
        info.pktSentNAKTotal,
        "pktRecvNAKTotal",
        info.pktRecvNAKTotal,
        /* Local Values */
        "pktSent",   
        info.pktSent,   
        "pktRecv",   
        info.pktRecv,   
        "pktSndLoss",
        info.pktSndLoss,
        "pktRcvLoss",
        info.pktRcvLoss,
        "pktRetrans",
        info.pktRetrans,
        "pktSentACK",
        info.pktSentACK,
        "pktRecvACK",
        info.pktRecvACK,
        "pktSentNAK",
        info.pktSentNAK,
        "pktRecvNAK",
        info.pktRecvNAK,
        "mbpsSendRate",
        info.mbpsSendRate,
        "mbpsRecvRate",
        info.mbpsRecvRate,
        /* Instant Values */
        "usPktSndPeriod",
        info.usPktSndPeriod,
        "pktFlowWindow",
        info.pktFlowWindow,
        "pktCongestionWindow",
        info.pktCongestionWindow,
        "pktFlightSize",
        info.pktFlightSize,
        "msRTT",
        info.msRTT,
        "mbpsBandwidth",
        info.mbpsBandwidth,
        "byteAvailSndBuf",
        info.byteAvailSndBuf,
        "byteAvailRcvBuf",
        info.byteAvailRcvBuf
    );

PY_CATCH_CXX(NULL)
}

/* 
    It looks like UDT::getsockopt doesn't check the buffer length.
    The big switch makes it easier to call from Python and prevents segfaults.
*/
PyObject* pyudt_socket_getsockopt(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX
    int level      = 0;
    int opt_name   = 0;
    int opt_len    = 0;
    
    if(!PyArg_ParseTuple(args, "ii", &level, &opt_name))
    {
        return NULL;
    }

    pyudt_socket_object* py_socket = ((pyudt_socket_object*)self);
     
    UDT::SOCKOPT switch_opt = (UDT::SOCKOPT)opt_name;   
    switch(switch_opt)
    {
        case UDT_MSS:
        case UDT_FC:
        case UDT_SNDBUF:
        case UDT_RCVBUF:
        case UDP_SNDBUF:
        case UDP_RCVBUF:
        case UDT_SNDTIMEO:
        case UDT_RCVTIMEO:
        {
            int opt_val = 0;
            if (UDT::getsockopt(
                py_socket->cc_socket, 
                level, switch_opt, &opt_val, &opt_len) == UDT::ERROR)
            {
                throw py_udt_error();
            }
            return Py_BuildValue("i", opt_val);
            break;
        }
        case UDT_SNDSYN:
        case UDT_RCVSYN:
        case UDT_RENDEZVOUS:
        case UDT_REUSEADDR:
        {
            bool opt_val = true;
            if (UDT::getsockopt(py_socket->cc_socket, 
                    level, switch_opt, &opt_val, &opt_len) == UDT::ERROR)
            {
                throw py_udt_error();
            }
            if(opt_val)
            {   
                Py_RETURN_TRUE;
            }
            else
            {
                Py_RETURN_FALSE;
            }
            break;
        }
        case UDT_CC:
        {
            PyErr_SetString(PyExc_NotImplementedError, "UTC_CC not implemented yet");
            return NULL;
            break;
        }
        case UDT_LINGER:
        {
            linger opt_val;
            opt_val.l_onoff = 0;
            opt_val.l_linger = 0;

            opt_len = sizeof(linger);
            if (UDT::getsockopt(py_socket->cc_socket, 
                    level, switch_opt, &opt_val, &opt_len) == UDT::ERROR)
            {
                throw py_udt_error();
            }
            return Py_BuildValue("(ii)", opt_val.l_onoff, opt_val.l_linger);
            break;
        }
        case UDT_MAXBW:
        {
            int64_t opt_val;
            if (UDT::getsockopt(py_socket->cc_socket, 
                    level, switch_opt, &opt_val, &opt_len) == UDT::ERROR)
            {
                throw py_udt_error();
            }
            return Py_BuildValue("l", opt_val);
            break;
        }
        default:
        {
            PyErr_SetString(PyExc_ValueError, "unknown socket option");
            return NULL;
        }
    }
PY_CATCH_CXX(NULL)
}

PyObject* pyudt_socket_setsockopt(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX
    int level      = 0;
    int opt_name   = 0;
    PyObject *opt_obj = NULL;
    
    if(!PyArg_ParseTuple(args, "iiO", &level, &opt_name, &opt_obj))
    {
        return NULL;
    }

    pyudt_socket_object* py_socket = ((pyudt_socket_object*)self);
     
    UDT::SOCKOPT switch_opt = (UDT::SOCKOPT)opt_name;   
    switch(switch_opt)
    {
        case UDT_MSS:
        case UDT_FC:
        case UDT_SNDBUF:
        case UDT_RCVBUF:
        case UDP_SNDBUF:
        case UDP_RCVBUF:
        case UDT_SNDTIMEO:
        case UDT_RCVTIMEO:
        {
            int opt_val = 0;
            if(!PyArg_ParseTuple(args, "iii", &level, &opt_name, &opt_val))
            {
                return NULL;
            }
            if (UDT::setsockopt(
                py_socket->cc_socket, 
                level, switch_opt, &opt_val, sizeof(int)) == UDT::ERROR)
            {
                throw py_udt_error();
            }
            Py_RETURN_NONE;
            break;
        }
        case UDT_SNDSYN:
        case UDT_RCVSYN:
        case UDT_RENDEZVOUS:
        case UDT_REUSEADDR:
        {
            bool opt_val = true;
            if(opt_obj == Py_True)
            {   
                opt_val = true;
            }
            else if(opt_obj == Py_False)
            {
                opt_val = false;
            }
            else
            {
                PyErr_SetString(PyExc_TypeError, "option must be boolean");
                return NULL;
            }
            
        
            if (UDT::setsockopt(py_socket->cc_socket, 
                    level, switch_opt, &opt_val, sizeof(bool)) == UDT::ERROR)
            {
                throw py_udt_error();
            }
            Py_RETURN_NONE;
            break;
        }
        case UDT_CC:
        {
            PyErr_SetString(PyExc_NotImplementedError, "UTC_CC not implemented yet");
            return NULL;
            break;
        }
        case UDT_LINGER:
        {
            linger opt_val;
            int opt_onoff;
            int opt_linger;

            if(!PyArg_ParseTuple(args, "ii(ii)", &level, &opt_name, &opt_onoff, &opt_linger))
            {
                return NULL;
            }

            opt_val.l_onoff   = opt_onoff;
            opt_val.l_linger  = opt_linger;

            if (UDT::setsockopt(py_socket->cc_socket, 
                    level, switch_opt, &opt_val, sizeof(linger)) == UDT::ERROR)
            {
                throw py_udt_error();
            }
            Py_RETURN_NONE;
            break;
        }
        case UDT_MAXBW:
        {
            int64_t opt_val;
            if(!PyArg_ParseTuple(args, "iil", &level, &opt_name, &opt_val))
            {   
                return NULL;
            }

            if (UDT::setsockopt(py_socket->cc_socket, 
                    level, switch_opt, &opt_val, sizeof(int64_t)) == UDT::ERROR)
            {
                throw py_udt_error();
            }
            return Py_BuildValue("l", opt_val);
            break;
        }
        default:
        {
            PyErr_SetString(PyExc_ValueError, "unknown socket option");
            return NULL;
        }
    }
PY_CATCH_CXX(NULL)
}


PyObject* pyudt_socket_fileno(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    pyudt_socket_object* py_socket = ((pyudt_socket_object*)self);

    if(!PyArg_ParseTuple(args, ""))
    {
        return NULL;
    }
    return Py_BuildValue("l", py_socket->cc_socket);

PY_CATCH_CXX(NULL)
}

static PyMethodDef pyudt_socket_methods[] = {
    {
        "accept",  
        (PyCFunction)pyudt_socket_accept, 
        METH_VARARGS, 
        "socket accept"
    },
    {
        "connect",   
        (PyCFunction)pyudt_socket_connect,  
        METH_VARARGS, 
        "socket connect"
    },
    {
        "close",   
        (PyCFunction)pyudt_socket_close,  
        METH_VARARGS, 
        "close the socket"
    },
    {
        "bind",    
        (PyCFunction)pyudt_socket_bind,  
        METH_VARARGS, 
        "socket bind"
    },
    {
        "listen",   
        (PyCFunction)pyudt_socket_listen,  
        METH_VARARGS, 
        "listen with backlog"
    },
    {
        "send",  
        (PyCFunction)pyudt_socket_send, 
        METH_VARARGS, 
        "send data"
    },
    {
        "sendmsg",  
        (PyCFunction)pyudt_socket_sendmsg, 
        METH_VARARGS, 
        "send msg"
    },
    {
        "recv",  
        (PyCFunction)pyudt_socket_recv, 
        METH_VARARGS, 
        "recv data"
    },
    {
        "recvmsg",  
        (PyCFunction)pyudt_socket_recvmsg, 
        METH_VARARGS, 
        "recv msg"
    },
    {
        "perfmon",  
        (PyCFunction)pyudt_socket_perfmon, 
        METH_VARARGS, 
        "get perfmon stats"
    },
    {
        "getsockopt",  
        (PyCFunction)pyudt_socket_getsockopt, 
        METH_VARARGS, 
        "get socket options"
    },
    {
        "setsockopt",  
        (PyCFunction)pyudt_socket_setsockopt, 
        METH_VARARGS, 
        "set socket options"
    },
    {
        "fileno",  
        (PyCFunction)pyudt_socket_fileno, 
        METH_VARARGS, 
        "get socket file descriptor"
    },
    {NULL}  /* Sentinel */
};

static PyGetSetDef pyudt_socket_getset[] = {
    {
        (char*)"family", 
        (getter)pyudt_socket_get_family, 
        NULL,
        (char*)"address family",
        NULL
    },
    {
        (char*)"type", 
        (getter)pyudt_socket_get_type, 
        NULL,
        (char*)"address type",
        NULL
    },
    {
        (char*)"proto", 
        (getter)pyudt_socket_get_proto, 
        NULL,
        (char*)"address protocol",
        NULL
    },
    {NULL}  /* Sentinel */
};


static PyMethodDef pyudt_methods[] = {
    {
        (char*)"startup", 
        (PyCFunction)pyudt_startup, 
        METH_VARARGS,
        (char*)"startup-up UDT library",
    },
    {
        (char*)"cleanup", 
        (PyCFunction)pyudt_cleanup, 
        METH_VARARGS,
        (char*)"clean-up UDT library",
    },
    {NULL, NULL, 0, NULL}   
};

static PyModuleDef udtmodule = {
  PyModuleDef_HEAD_INIT,
  "_udt",
  "UDT: UDP-based Data Transfer Library",
  -1,
  pyudt_methods,
  NULL, NULL, NULL, NULL
};


static PyTypeObject pyudt_socket_type = {
    PyObject_HEAD_INIT(NULL)
    "_udt.socket",                  /* tp_name */
    sizeof(pyudt_socket_object),    /* tp_basicsize */
    0,                              /* tp_itemsize */
    0,                              /* tp_dealloc */
    0,                              /* tp_print */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_compare */
    0,                              /* tp_repr */
    0,                              /* tp_as_number */
    0,                              /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash */
    0,                              /* tp_call */
    0,                              /* tp_str */
    0,                              /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,/* tp_flags */
    "UDT socket",                   /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    0,                              /* tp_iter */
    0,                              /* tp_iternext */
    pyudt_socket_methods,           /* tp_methods */
    0,                              /* tp_members */
    pyudt_socket_getset,            /* tp_getset */
    0,                              /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    (initproc)pyudt_socket_init,    /* tp_init */
    0,                              /* tp_alloc */
    0,                              /* tp_new */

};

PyObject* pyudt_socket_accept(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX
    // char *address;
    // int port;
    int namelen;
    
    sockaddr_in client_addr;

    pyudt_socket_object* py_socket  = ((pyudt_socket_object*)self);

    if(!PyArg_ParseTuple(args, ""))
    {
        return NULL;
    }

    UDTSOCKET cc_client = 0;
    {
        AutoGILCallOut g;
        cc_client = UDT::accept(
            py_socket->cc_socket, 
            (sockaddr*)&client_addr, 
            &namelen
        );
    }

    if (cc_client == UDT::ERROR)
    {
        throw py_udt_error();
    }

    pyudt_socket_object* py_client = (pyudt_socket_object*)PyObject_New(
            pyudt_socket_object, 
            &pyudt_socket_type
    );
    
    AutoDecref dec((PyObject*)py_client);

    py_client->cc_socket = cc_client;
    py_client->family = client_addr.sin_family;
    py_client->type =  py_socket->type;         // FIXME
    py_client->proto = py_socket->proto;        // FIXME

    PyObject *ret = Py_BuildValue(
        "N(si)", 
        py_client, 
        inet_ntoa(client_addr.sin_addr),
        client_addr.sin_port
    );
    
    if(ret == NULL)
    {
        return NULL;
    }
    dec.ok();
    return ret;


PY_CATCH_CXX(NULL)
}

/* FIXME - should accept None. */
static PyObject* pyudt_epoll_add_usock(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX
    int eid = ((pyudt_epoll_object*)self)->eid;
    int events = 0;
    int fileno = 0;

    if(!PyArg_ParseTuple(args, "ii", &fileno, &events))
    {
        return NULL;
    }
    
    int rv = UDT::epoll_add_usock(eid, fileno, &events);

    if(rv < 0)
    {
        throw py_udt_error();
    }
    return Py_BuildValue("i", rv);
PY_CATCH_CXX(NULL)
}


/* FIXME - should accept None. */
static PyObject* pyudt_epoll_add_ssock(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX
    int eid = ((pyudt_epoll_object*)self)->eid;
    int events = 0;
    int fileno = 0;

    if(!PyArg_ParseTuple(args, "ii", &fileno, &events))
    {
        return NULL;
    }
    
    int rv = UDT::epoll_add_ssock(eid, fileno, &events);

    if(rv < 0)
    {
        throw py_udt_error();
    }
    return Py_BuildValue("i", rv);
PY_CATCH_CXX(NULL)
}

/* FIXME - should accept None. */
static PyObject* pyudt_epoll_remove_usock(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX
    int eid = ((pyudt_epoll_object*)self)->eid;
    int events = 0;
    int fileno = 0;

    if(!PyArg_ParseTuple(args, "ii", &fileno, &events))
    {
        return NULL;
    }

    int rv = UDT::epoll_remove_usock(eid, fileno);

    if(rv < 0)
    {
        throw py_udt_error();
    }
    return Py_BuildValue("i", rv);
PY_CATCH_CXX(NULL)
}


/* FIXME - should accept None. */
static PyObject* pyudt_epoll_remove_ssock(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX
    int eid = ((pyudt_epoll_object*)self)->eid;
    int events = 0;
    int fileno = 0;

    if(!PyArg_ParseTuple(args, "ii", &fileno, &events))
    {
        return NULL;
    }
    
    int rv = UDT::epoll_remove_ssock(eid, fileno);

    if(rv < 0)
    {
        throw py_udt_error();
    }
    return Py_BuildValue("i", rv);
PY_CATCH_CXX(NULL)
}

static PyObject* pyudt_epoll_wait(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    PyObject *r_usock_set = NULL;
    PyObject *w_usock_set = NULL;
    
    PyObject *r_ssock_set = NULL;
    PyObject *w_ssock_set = NULL;

    int64_t timeout = 0;
    
    int eid = ((pyudt_epoll_object*)self)->eid;

    if(!PyArg_ParseTuple(args, "i", &timeout))
    {
        return NULL;
    }

    r_usock_set = PySet_New(0);
    if(!r_usock_set) return NULL;
    AutoDecref dec1(r_usock_set);

    w_usock_set = PySet_New(0);
    if(!w_usock_set) return NULL;
    AutoDecref dec2(w_usock_set);

    r_ssock_set = PySet_New(0);
    if(!r_ssock_set) return NULL;
    AutoDecref dec3(r_ssock_set);

    w_ssock_set = PySet_New(0);
    if(!w_ssock_set) return NULL;
    AutoDecref dec4(w_ssock_set);

    std::set<UDTSOCKET> r_usock_out;    
    std::set<UDTSOCKET> w_usock_out;

    std::set<SYSSOCKET> r_ssock_out;    
    std::set<SYSSOCKET> w_ssock_out;

    {
        AutoGILCallOut g;
        if(UDT::epoll_wait(eid, &r_usock_out, &w_usock_out, timeout) < 0)
        {
            throw py_udt_error();
        }
    }
    
    std::set<UDTSOCKET>::const_iterator usock_iter;
    for(usock_iter = r_usock_out.begin(); usock_iter != r_usock_out.end(); ++usock_iter)
    {

        PyObject *i = PyLong_FromLong(*usock_iter);

        if(!i)
        {
            return NULL;
        }

        if(PySet_Add(r_usock_set, i) == -1)
        {
            return NULL;
        }
    }

    for(usock_iter = w_usock_out.begin(); usock_iter != w_usock_out.end(); ++usock_iter)
    {

        PyObject *i = PyLong_FromLong(*usock_iter);

        if(!i)
        {
            return NULL;
        }

        if(PySet_Add(w_usock_set, i) == -1)
        {
            return NULL;
        }
    }

    std::set<UDTSOCKET>::const_iterator ssock_iter;
    for(ssock_iter = r_ssock_out.begin(); ssock_iter != r_ssock_out.end(); ++ssock_iter)
    {

        PyObject *i = PyLong_FromLong(*ssock_iter);

        if(!i)
        {
            return NULL;
        }

        if(PySet_Add(r_ssock_set, i) == -1)
        {
            return NULL;
        }
    }

    for(ssock_iter = w_ssock_out.begin(); ssock_iter != w_ssock_out.end(); ++ssock_iter)
    {

        PyObject *i = PyLong_FromLong(*ssock_iter);

        if(!i)
        {
            return NULL;
        }

        if(PySet_Add(w_ssock_set, i) == -1)
        {
            return NULL;
        }
    }

    PyObject *ret = Py_BuildValue(
        "NNNN", 
        r_usock_set,
        w_usock_set,
        r_ssock_set,
        w_ssock_set
    );

    dec1.ok();
    dec2.ok();
    dec3.ok();
    dec4.ok();

    return ret;

PY_CATCH_CXX(NULL)
}

PyMODINIT_FUNC
PyInit__udt(void)
{
    PyObject *module;

    pyudt_socket_type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&pyudt_socket_type) < 0)
        return NULL;

    pyudt_epoll_type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&pyudt_epoll_type) < 0)
        return NULL;

    module = PyModule_Create(&udtmodule);
    if (module == NULL)
    {
        return NULL;
    }

    Py_INCREF(&pyudt_socket_type);
    Py_INCREF(&pyudt_epoll_type);
    PyModule_AddObject(module, "socket", (PyObject *)&pyudt_socket_type);
    PyModule_AddObject(module, "epoll",  (PyObject *)&pyudt_epoll_type);

    if(PyModule_AddIntConstant(module, "UDT_MSS",         UDT_MSS)      == -1   ) {return NULL;}
    if(PyModule_AddIntConstant(module, "UDT_SNDSYN",      UDT_SNDSYN)   == -1   ) {return NULL;}
    if(PyModule_AddIntConstant(module, "UDT_RCVSYN",      UDT_RCVSYN)   == -1   ) {return NULL;}
    if(PyModule_AddIntConstant(module, "UDT_CC",          UDT_CC)       == -1   ) {return NULL;}
    if(PyModule_AddIntConstant(module, "UDT_FC",          UDT_FC)       == -1   ) {return NULL;}
    if(PyModule_AddIntConstant(module, "UDT_SNDBUF",      UDT_SNDBUF)   == -1   ) {return NULL;}
    if(PyModule_AddIntConstant(module, "UDT_RCVBUF",      UDT_RCVBUF)   == -1   ) {return NULL;}
    if(PyModule_AddIntConstant(module, "UDT_LINGER",      UDT_LINGER)   == -1   ) {return NULL;}
    if(PyModule_AddIntConstant(module, "UDP_SNDBUF",      UDP_SNDBUF)   == -1   ) {return NULL;}
    if(PyModule_AddIntConstant(module, "UDP_RCVBUF",      UDP_RCVBUF)   == -1   ) {return NULL;}
    if(PyModule_AddIntConstant(module, "UDT_MAXMSG",      UDT_MAXMSG)   == -1   ) {return NULL;}
    if(PyModule_AddIntConstant(module, "UDT_MSGTTL",      UDT_MSGTTL)   == -1   ) {return NULL;}
    if(PyModule_AddIntConstant(module, "UDT_RENDEZVOUS",  UDT_RENDEZVOUS) == -1 ) {return NULL;}
    if(PyModule_AddIntConstant(module, "UDT_SNDTIMEO",    UDT_SNDTIMEO) == -1   ) {return NULL;}
    if(PyModule_AddIntConstant(module, "UDT_RCVTIMEO",    UDT_RCVTIMEO) == -1   ) {return NULL;}
    if(PyModule_AddIntConstant(module, "UDT_REUSEADDR",   UDT_REUSEADDR) == -1  ) {return NULL;}
    if(PyModule_AddIntConstant(module, "UDT_MAXBW",       UDT_MAXBW)    == -1   ) {return NULL;}
    if(PyModule_AddIntConstant(module, "UDT_EPOLL_IN",    UDT_EPOLL_IN) == -1   ) {return NULL;}
    if(PyModule_AddIntConstant(module, "UDT_EPOLL_OUT",   UDT_EPOLL_OUT) == -1  ) {return NULL;}
    if(PyModule_AddIntConstant(module, "UDT_EPOLL_ERR",   UDT_EPOLL_ERR) == -1  ) {return NULL;}

    return module;
}
