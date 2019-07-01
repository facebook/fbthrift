cimport cython
from thrift.py3.exceptions cimport create_py_exception
from thrift.py3.common import Protocol
from thrift.py3.common cimport Protocol2PROTOCOL_TYPES
cimport thrift.py3.ssl as thrift_ssl
from libcpp.string cimport string
from libc.stdint cimport uint64_t
from cython.operator cimport dereference as deref
from folly.futures cimport bridgeFutureWith
from folly cimport cFollyTry, cFollyPromise
from folly.executor cimport get_executor
from cpython.ref cimport PyObject
from libcpp cimport nullptr
import asyncio
import ipaddress
import os
from socket import SocketKind

cdef object proxy_factory = None


cpdef object get_proxy_factory():
    return proxy_factory


def install_proxy_factory(factory):
    global proxy_factory
    proxy_factory = factory


@cython.auto_pickle(False)
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


async def _no_op():
    pass


@cython.auto_pickle(False)
cdef class _AsyncResolveCtxManager:
    """This class just handles resolving of hostnames passed to get_client
       by creating a wrapping async context manager"""
    cdef object clientKlass
    cdef object kws
    cdef object ctx

    def __init__(self, clientKlass, *, **kws):
        self.clientKlass = clientKlass
        self.kws = kws

    async def __aenter__(self):
        loop = asyncio.get_event_loop()
        result = await loop.getaddrinfo(
            self.kws['host'],
            self.kws['port'],
            type=SocketKind.SOCK_STREAM
        )
        self.kws['host'] = result[0][4][0]
        self.ctx = get_client(self.clientKlass, **self.kws)
        return await self.ctx.__aenter__()

    def __aexit__(self, *exc_info):
        if self.ctx:
            awaitable = self.ctx.__aexit__(*exc_info)
            self.ctx = None
            return awaitable
        return _no_op()


def get_client(
    clientKlass,
    *,
    host='::1',
    int port=-1,
    path=None,
    double timeout=1,
    headers=None,
    ClientType client_type = ClientType.THRIFT_HEADER_CLIENT_TYPE,
    protocol = Protocol.COMPACT,
    thrift_ssl.SSLContext ssl_context=None,
    double ssl_timeout=1
):
    if not isinstance(protocol, Protocol):
        raise TypeError(f'protocol={protocol} is not a valid {Protocol}')
    loop = asyncio.get_event_loop()
    # This is to prevent calling get_client at import time at module scope
    assert loop.is_running(), "Eventloop is not running"
    assert issubclass(clientKlass, Client), "Must be a py3 thrift client"

    cdef uint32_t _timeout_ms = int(timeout * 1000)
    cdef uint32_t _ssl_timeout_ms = int(ssl_timeout * 1000)
    cdef PROTOCOL_TYPES proto = Protocol2PROTOCOL_TYPES(protocol)
    cdef string cstr

    endpoint = b''
    if client_type is ClientType.THRIFT_HTTP_CLIENT_TYPE:
        if path is None:
            raise TypeError("use path='/endpoint' when using ClientType.THRIFT_HTTP_CLIENT_TYPE")
        endpoint = os.fsencode(path)  # means we can accept bytes/str/Path objects
        path = None

    if port == -1 and path is None:
        raise ValueError('path or port must be set')

    # See if what we were given is an ip or hostname, if not an ip return a resolver
    if not path and isinstance(host, str):
        try:
            ipaddress.ip_address(host)
        except ValueError:
            return _AsyncResolveCtxManager(
                clientKlass,
                host=host,
                port=port,
                path=endpoint,
                timeout=timeout,
                headers=headers,
                client_type=client_type,
                protocol=protocol,
                ssl_context=ssl_context,
                ssl_timeout=ssl_timeout
            )

    host = str(host)  # Accept ipaddress objects
    client = clientKlass()

    if path:
        fspath = os.fsencode(path)
        bridgeFutureWith[cRequestChannel_ptr](
            (<Client>client)._executor,
            createThriftChannelUnix(move_string(fspath), _timeout_ms, client_type, proto),
            requestchannel_callback,
            <PyObject *> client
        )
    elif ssl_context:
        cstr = <bytes> host.encode('utf-8')
        bridgeFutureWith[cRequestChannel_ptr](
            (<Client>client)._executor,
            thrift_ssl.createThriftChannelTCP(
                ssl_context._cpp_obj,
                move_string(cstr),
                port,
                _timeout_ms,
                _ssl_timeout_ms,
                client_type,
                proto,
                move_string(endpoint)
            ),
            requestchannel_callback,
            <PyObject *> client
        )
    else:
        cstr = <bytes> host.encode('utf-8')
        bridgeFutureWith[cRequestChannel_ptr](
            (<Client>client)._executor,
            createThriftChannelTCP(
                move_string(cstr),
                port,
                _timeout_ms,
                client_type,
                proto,
                move_string(endpoint)
            ),
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
