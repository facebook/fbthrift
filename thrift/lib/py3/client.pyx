from thrift.py3.client cimport (
    Client,
    cRequestChannel_ptr,
    createThriftChannelTCP,
    createThriftChannelUnix,
)
from thrift.py3.exceptions cimport raise_py_exception
from libcpp.string cimport string
from cython.operator cimport dereference as deref
from folly.futures cimport bridgeFutureWith
from folly cimport cFollyTry
from cpython.ref cimport PyObject
from libcpp cimport nullptr
import asyncio
import os


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
    cdef const type_info* _typeid(self):
        return NULL


def get_client(
        clientKlass, *,
        str host='::1',
        int port=0,
        str path=None,
        float timeout=1,
        headers=None):
    loop = asyncio.get_event_loop()
    # This is to prevent calling get_client at import time at module scope
    assert loop.is_running(), "Eventloop is not running"
    assert issubclass(clientKlass, Client), "Must be a py3 thrift client"
    cdef string cstr
    cdef int _timeout = int(timeout * 1000)
    client = clientKlass()
    if port != 0:
        if path is not None:
            raise Exception("cannot specify both path and host/port")
        cstr = <bytes> host.encode('idna')
        bridgeFutureWith[cRequestChannel_ptr](
            (<Client>client)._executor,
            createThriftChannelTCP(cstr, port, _timeout),
            requestchannel_callback,
            <PyObject *> client
        )
    elif path is not None:
        cstr = <bytes> os.fsencode(path)
        bridgeFutureWith[cRequestChannel_ptr](
            (<Client>client)._executor,
            createThriftChannelUnix(cstr, _timeout),
            requestchannel_callback,
            <PyObject *> client
        )
    else:
        raise Exception("must specify either port or path arguments")
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
        try:
            raise_py_exception(result.exception())
        except Exception as ex:
            future.set_exception(ex)
    else:
        client._cRequestChannel = result.value()
        future.set_result(None)
