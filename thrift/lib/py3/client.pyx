from thrift.py3.client cimport (
    Client, cRequestChannel_ptr, createThriftChannel,
)
from libcpp.string cimport string
from cython.operator cimport dereference as deref
from folly.futures cimport bridgeFutureWith
from folly cimport cFollyTry
from cpython.ref cimport PyObject
from libcpp cimport nullptr
import asyncio


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


def get_client(clientKlass, *, str host='::1', int port, float timeout=1, headers=None):
    loop = asyncio.get_event_loop()
    # This is to prevent calling get_client at import time at module scope
    assert loop.is_running(), "Eventloop is not running"
    assert issubclass(clientKlass, Client), "Must by a py3 thrift client"
    cdef string chost = <bytes> host.encode('idna')
    cdef int _timeout = int(timeout * 1000)
    client = clientKlass()
    bridgeFutureWith[cRequestChannel_ptr](
        (<Client>client)._executor,
        createThriftChannel(chost, port, _timeout),
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
        try:
            result.exception().throw_exception()
        except Exception as ex:
            future.set_exception(ex)
    else:
        client._cRequestChannel = result.value()
        future.set_result(None)
