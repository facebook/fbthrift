from thrift.py3.client cimport (
    Client,
    cRequestChannel_ptr,
    createThriftChannelTCP,
    createThriftChannelUnix,
)
from thrift.py3.exceptions cimport create_py_exception
from libcpp.string cimport string
from cython.operator cimport dereference as deref
from folly.futures cimport bridgeFutureWith
from folly cimport cFollyTry, cFollyPromise
from folly.executor cimport get_executor
from folly.range cimport StringPiece
from cpython.ref cimport PyObject
from libcpp cimport nullptr
import asyncio
import os
from socket import SocketKind

cdef object proxy_factory = None


cdef object get_proxy_factory():
    return proxy_factory


def install_proxy_factory(factory):
    global proxy_factory
    proxy_factory = factory


cdef class Client:
    """
    Base class for all thrift clients
    """
    def __cinit__(Client self):
        self._executor = get_executor()

    cdef const type_info* _typeid(self):
        return NULL

    cdef bind_client(Client self, cRequestChannel_ptr&& channel):
        destroyInEventBaseThread(move(channel))


cdef extern from "<stdexcept>" namespace "std" nogil:
    cdef cppclass cruntime_error "std::runtime_error":
        cruntime_error(string& what)


cdef class _ResolvePromise:
    cdef cFollyPromise[string] cPromise

    def __init__(self, hostname, port):
        loop = asyncio.get_event_loop()
        asyncio.ensure_future(
            loop.getaddrinfo(hostname, port, type=SocketKind.SOCK_STREAM)
        ).add_done_callback(self._callback)

    def _callback(_ResolvePromise self, fut):
        ex = fut.exception()
        cdef string ip
        if ex:
            self.cPromise.setException(cruntime_error(repr(ex).encode('utf-8')))
        else:
            res = fut.result()
            ip = res[0][4][0].encode('utf-8')
            self.cPromise.setValue(ip)


def get_client(clientKlass, *, host='::1', int port=-1, path=None, float timeout=1, headers=None):
    loop = asyncio.get_event_loop()
    # This is to prevent calling get_client at import time at module scope
    assert loop.is_running(), "Eventloop is not running"
    assert issubclass(clientKlass, Client), "Must be a py3 thrift client"
    host = str(host)  # Accept ipaddress objects
    cdef int _timeout = int(timeout * 1000)

    if port == -1 and path is None:
        raise ValueError('path or port must be set')

    client = clientKlass()

    if path:
        fspath = os.fsencode(path)
        bridgeFutureWith[cRequestChannel_ptr](
            (<Client>client)._executor,
            createThriftChannelUnix(StringPiece(fspath, len(fspath)), _timeout),
            requestchannel_callback,
            <PyObject *> client
        )
    else:
        p = _ResolvePromise(host, port)
        bridgeFutureWith[cRequestChannel_ptr](
            (<Client>client)._executor,
            createThriftChannelTCP(p.cPromise.getFuture(), port, _timeout),
            requestchannel_callback,
            <PyObject *> client
        )
    if headers:
        for key, value in headers.items():
            client.set_persistent_header(key, value)

    factory = get_proxy_factory()
    proxy = factory(clientKlass) if factory else None
    return proxy(client) if proxy else client


cdef void requestchannel_callback(
    cFollyTry[cRequestChannel_ptr]&& result,
    PyObject* userData,
):
    cdef Client client = <object> userData
    future = client._connect_future
    if result.hasException():
        future.set_exception(create_py_exception(result.exception(), None))
    else:
        client.bind_client(move(result.value()))
        future.set_result(None)
