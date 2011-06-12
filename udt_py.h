#include <Python.h>
#include <udt.h>
#include <map>

typedef struct py_udt_error        : std::exception {} py_udt_error;
typedef struct cc_general_error    : std::exception {} cc_general_error;

/* 
    FIXME - Need to implement GIL handling 
*/
class AutoGILCallOut
{
    public:
        AutoGILCallOut();
        ~AutoGILCallOut();

    private:
        PyThreadState *state;
};

class AutoGILCallBack
{
    public:
        AutoGILCallBack();
        ~AutoGILCallBack();
    
    private:
        PyGILState_STATE  state;
};

class AutoDecref
{
    public:
        AutoDecref(PyObject *o);
        ~AutoDecref();
        void ok();

    private:
        PyObject *ptr;
};

typedef struct 
{
    PyObject_HEAD;
    UDTSOCKET cc_socket;
    int family;
    int type; 
    int proto;

} pyudt_socket_object;

typedef struct 
{
    PyObject_HEAD;
    int eid;
} pyudt_epoll_object;

class RecvBuffer
{
    public:
        RecvBuffer(unsigned int size);
        ~RecvBuffer();

        char *get_head();
        unsigned int get_max_len();
        unsigned int get_buf_len();
        unsigned int set_buf_len(unsigned int len);

    private:
        char *head;
        unsigned int max_buf_len;
        unsigned int buf_len;
        
    
};
