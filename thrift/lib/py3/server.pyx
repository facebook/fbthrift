from libcpp.memory cimport unique_ptr, make_unique, shared_ptr
from libc.string cimport const_uchar
from cython.operator cimport dereference as deref
from libc.stdint cimport uint64_t
from thrift.py3.server cimport (
    cSSLPolicy, SSLPolicy__DISABLED, SSLPolicy__PERMITTED, SSLPolicy__REQUIRED
)
from folly.iobuf cimport IOBuf, move

import asyncio
import collections
import inspect
import ipaddress

from enum import Enum


SocketAddress = collections.namedtuple('SocketAddress', 'ip port path')


cdef inline _get_SocketAddress(const cfollySocketAddress* sadr):
    if sadr.isFamilyInet():
        ip = ipaddress.ip_address(sadr.getAddressStr().decode('utf-8'))
        return SocketAddress(ip=ip, port=sadr.getPort(), path=None)
    return SocketAddress(ip=None, port=None, path=sadr.getPath())


def pass_context(func):
    """Decorate a handler as wanting the Request Context"""
    func.pass_context = True
    return func


class SSLPolicy(Enum):
    DISABLED = <int> (SSLPolicy__DISABLED)
    PERMITTED = <int> (SSLPolicy__PERMITTED)
    REQUIRED = <int> (SSLPolicy__REQUIRED)


cdef class ServiceInterface:
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.loop = asyncio.get_event_loop()


cdef class ThriftServer:
    def __cinit__(self):
        self.server = make_unique[cThriftServer]()

    def __init__(self, ServiceInterface handler, port):
        self.loop = asyncio.get_event_loop()
        self.handler = handler

        # Figure out which methods want context and mark them on the handler
        for name, method in inspect.getmembers(handler,
                                               inspect.iscoroutinefunction):
            if hasattr(method, 'pass_context'):
                setattr(handler, f'_pass_context_{name}', True)

        self.server.get().setInterface(handler.interface_wrapper)
        self.server.get().setPort(port)

    async def serve(self):
        def _serve():
            with nogil:
                self.server.get().serve()
        try:
            await self.loop.run_in_executor(None, _serve)
        except Exception:
            print("Exception In Server")
            self.server.get().stop()
            raise

    def set_ssl_policy(self, policy):
        cdef cSSLPolicy cPolicy
        if policy == SSLPolicy.DISABLED:
            cPolicy = SSLPolicy__DISABLED
        elif policy == SSLPolicy.PERMITTED:
            cPolicy = SSLPolicy__PERMITTED
        elif policy == SSLPolicy.REQUIRED:
            cPolicy = SSLPolicy__REQUIRED
        self.server.get().setSSLPolicy(cPolicy)

    def stop(self):
        self.server.get().stop()


cdef class ConnectionContext:
    @staticmethod
    cdef ConnectionContext create(Cpp2ConnContext* ctx):
        inst = <ConnectionContext>ConnectionContext.__new__(ConnectionContext)
        inst._ctx = ctx
        inst._peer_address = _get_SocketAddress(ctx.getPeerAddress())
        return inst

    @property
    def peer_address(ConnectionContext self):
        return self._peer_address

    @property
    def is_tls(ConnectionContext self):
        return self._ctx.isTls()

    @property
    def peer_common_name(ConnectionContext self):
        return self._ctx.getPeerCommonName().decode('utf-8')

    @property
    def peer_certificate(ConnectionContext self):
        cdef const_uchar* data
        cdef unique_ptr[IOBuf] der
        cdef shared_ptr[X509] cert
        cdef uint64_t length
        if self._ctx.isTls():
            cert = self._ctx.getPeerCertificate()
            der = move(derEncode(deref(cert.get())))
            length = der.get().length()
            data = der.get().data()
            return data[:length]
        return None


cdef class RequestContext:
    @staticmethod
    cdef RequestContext create(Cpp2RequestContext* ctx):
        inst = <RequestContext>RequestContext.__new__(RequestContext)
        inst._ctx = ctx
        inst._c_ctx = ConnectionContext.create(ctx.getConnectionContext())
        return inst

    @property
    def connection_context(self):
        return self._c_ctx
