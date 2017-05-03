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

cdef class Client:
    """
    Base class for all thrift clients
    """
    cdef const type_info* _typeid(self):
        return NULL


def get_client(clientKlass, *, str host='::1', int port, float timeout=1):
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
    return client


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
