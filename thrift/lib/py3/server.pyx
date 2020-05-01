# Copyright (c) Facebook, Inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from cpython.version cimport PY_VERSION_HEX
from libcpp.memory cimport unique_ptr, shared_ptr, make_shared
from libc.string cimport const_uchar
from cython.operator cimport dereference as deref
from libc.stdint cimport uint64_t
from folly.iobuf cimport from_unique_ptr as create_IOBuf
from cpython.ref cimport PyObject
from folly.executor cimport get_executor
from folly.range cimport StringPiece

import asyncio
import collections
import functools
import inspect
import ipaddress
from pathlib import Path
import os

from enum import Enum
from thrift.py3.common import Priority, Headers

SocketAddress = collections.namedtuple('SocketAddress', 'ip port path')

if PY_VERSION_HEX >= 0x030702F0:  # 3.7.2 Final
    from contextvars import ContextVar
    # don't include in the module dict, so only cython can set it
    THRIFT_REQUEST_CONTEXT = ContextVar('ThriftRequestContext')
    get_context = THRIFT_REQUEST_CONTEXT.get
else:
    def get_context(default=None):
        raise RuntimeError('get_context requires python >= 3.7.2')


cdef inline _get_SocketAddress(const cfollySocketAddress* sadr):
    if sadr.isFamilyInet():
        ip = ipaddress.ip_address(sadr.getAddressStr().decode('utf-8'))
        return SocketAddress(ip=ip, port=sadr.getPort(), path=None)
    return SocketAddress(ip=None, port=None, path=Path(
            os.fsdecode(sadr.getPath())
        )
    )


def pass_context(func):
    """Decorate a handler as wanting the Request Context"""
    if PY_VERSION_HEX < 0x030702F0:  # 3.7.2 Final
        func.pass_context = True
        return func

    @functools.wraps(func)
    def decorated(self, *args, **kwargs):
        ctx = get_context(None)
        if ctx is None:
            return func(self, *args, **kwargs)
        return func(self, get_context(), *args, **kwargs)
    return decorated



class SSLPolicy(Enum):
    DISABLED = <int> (SSLPolicy__DISABLED)
    PERMITTED = <int> (SSLPolicy__PERMITTED)
    REQUIRED = <int> (SSLPolicy__REQUIRED)


cdef class AsyncProcessorFactory:
    pass


cdef class ServiceInterface(AsyncProcessorFactory):
    async def __aenter__(self):
        # Establish async context managers as a way for end users to async initalize
        # internal structures used by Service Handlers.
        return self

    async def __aexit__(self, *exc_info):
        # Same as above, but allow end users to define things to be cleaned up
        pass


def getServiceName(ServiceInterface svc not None):
    processor = deref(svc._cpp_obj).getProcessor()
    gen_proc = dynamic_cast_gen(processor.get())
    if not gen_proc:
        raise TypeError('processor was not a GeneratedAsyncProcessor')
    cdef const char* name = gen_proc.getServiceName()
    return (<bytes>name).decode('utf-8')


cdef void handleAddressCallback(PyObject* future, cfollySocketAddress address):
    (<object>future).set_result(_get_SocketAddress(&address))


cdef class ThriftServer:
    def __cinit__(self):
        self.server = make_shared[cThriftServer]()

    def __init__(self, AsyncProcessorFactory handler, int port=0, ip=None, path=None):
        self.loop = asyncio.get_event_loop()
        self.factory = handler

        if handler._cpp_obj:
            self.server.get().setProcessorFactory(handler._cpp_obj)
        else:
            raise RuntimeError(
                'The handler is not valid, it has no C++ handler. Maybe its not a '
                'generated ServiceInterface?'
            )
        if path:
            fspath = os.fsencode(path)
            self.server.get().setAddress(
                makeFromPath(
                    StringPiece(fspath, len(fspath))
                )
            )
        elif ip:
            # We stringify to accept python ipaddress objects
            self.server.get().setAddress(str(ip).encode('utf-8'), port)
        else:
            self.server.get().setPort(port)
        self.address_future = self.loop.create_future()
        self.server.get().setServerEventHandler(
            make_shared[Py3ServerEventHandler](
                get_executor(),
                object_partial(handleAddressCallback, <PyObject*> self.address_future)
            )
        )

    async def serve(self):
        def _serve():
            with nogil:
                self.server.get().serve()
        try:
            await self.loop.run_in_executor(None, _serve)
            self.address_future.cancel()
        except asyncio.CancelledError:
            try:
                await self.get_address()
            finally:
                self.server.get().stop()
            raise
        except Exception as e:
            self.server.get().stop()
            # If somebody is waiting on get_address and the server died
            # then we should forward this exception over to that future.
            if not self.address_future.done():
                self.address_future.set_exception(e)
            raise

    def get_address(self):
        return asyncio.shield(self.address_future)

    def get_active_requests(self):
        return self.server.get().getActiveRequests()

    def get_max_requests(self):
        return self.server.get().getMaxRequests()

    def set_max_requests(self, max_requests):
        self.server.get().setMaxRequests(max_requests)

    def get_max_connections(self):
        return self.server.get().getMaxConnections()

    def set_max_connections(self, max_connections):
        self.server.get().setMaxConnections(max_connections)

    def get_listen_backlog(self):
        return self.server.get().getListenBacklog()

    def set_listen_backlog(self, listen_backlog):
        self.server.get().setListenBacklog(listen_backlog)

    def set_io_worker_threads(self, num):
        self.server.get().setNumIOWorkerThreads(num)

    def get_io_worker_threads(self):
        return self.server.get().getNumIOWorkerThreads()

    def set_cpu_worker_threads(self, num):
        self.server.get().setNumCPUWorkerThreads(num)

    def get_cpu_worker_threads(self):
        return self.server.get().getNumCPUWorkerThreads()

    def set_ssl_handshake_worker_threads(self, num):
        self.server.get().setNumSSLHandshakeWorkerThreads(num)

    def get_ssl_handshake_worker_threads(self):
        return self.server.get().getNumSSLHandshakeWorkerThreads()

    def get_ssl_policy(self):
        cdef cSSLPolicy cPolicy = self.server.get().getSSLPolicy()
        if cPolicy == SSLPolicy__DISABLED:
            return SSLPolicy.DISABLED
        elif cPolicy == SSLPolicy__PERMITTED:
            return SSLPolicy.PERMITTED
        elif cPolicy == SSLPolicy__REQUIRED:
            return SSLPolicy.REQUIRED
        else:
            raise RuntimeError("Unknown SSLPolicy defined.")

    def set_ssl_policy(self, policy):
        cdef cSSLPolicy cPolicy
        if policy == SSLPolicy.DISABLED:
            cPolicy = SSLPolicy__DISABLED
        elif policy == SSLPolicy.PERMITTED:
            cPolicy = SSLPolicy__PERMITTED
        elif policy == SSLPolicy.REQUIRED:
            cPolicy = SSLPolicy__REQUIRED
        else:
            raise RuntimeError("Unknown SSLPolicy defined.")
        self.server.get().setSSLPolicy(cPolicy)

    def set_allow_plaintext_on_loopback(self, enabled):
        self.server.get().setAllowPlaintextOnLoopback(enabled);

    def is_plaintext_allowed_on_loopback(self):
        return self.server.get().isPlaintextAllowedOnLoopback();

    def set_idle_timeout(self, seconds):
        self.server.get().setIdleTimeout(milliseconds(<int64_t>(seconds * 1000)))

    def get_idle_timeout(self):
        return self.server.get().getIdleTimeout().count() / 1000

    def set_queue_timeout(self, seconds):
        self.server.get().setQueueTimeout(milliseconds(<int64_t>(seconds * 1000)))

    def get_queue_timeout(self):
        return self.server.get().getQueueTimeout().count() / 1000

    def stop(self):
        self.server.get().stop()


cdef class ConnectionContext:
    @staticmethod
    cdef ConnectionContext create(Cpp2ConnContext* ctx):
        inst = <ConnectionContext>ConnectionContext.__new__(ConnectionContext)
        if ctx:
            inst._ctx = ctx
            inst._peer_address = _get_SocketAddress(ctx.getPeerAddress())
        return inst

    @property
    def peer_address(ConnectionContext self):
        return self._peer_address

    @property
    def peer_common_name(ConnectionContext self):
        return self._ctx.getPeerCommonName().decode('utf-8')

    @property
    def security_protocol(ConnectionContext self):
        return self._ctx.getSecurityProtocol().decode('utf-8')

    @property
    def peer_certificate(ConnectionContext self):
        cdef const_uchar* data
        cdef shared_ptr[X509] cert
        cdef uint64_t length
        cert = self._ctx.getPeerCertificate()
        if cert.get():
            iobuf = create_IOBuf(derEncode(deref(cert.get())))
            if iobuf.is_chained:
                return b''.join(iobuf)
            return bytes(iobuf)
        return None


cdef class ReadHeaders(Headers):
    @staticmethod
    cdef create(RequestContext ctx):
        inst = <ReadHeaders>ReadHeaders.__new__(ReadHeaders)
        inst._parent = ctx
        return inst

    cdef const map[string, string]* _getMap(self):
        return &self._parent._ctx.getHeader().getHeaders()


cdef class WriteHeaders(Headers):
    @staticmethod
    cdef create(RequestContext ctx):
        inst = <WriteHeaders>WriteHeaders.__new__(WriteHeaders)
        inst._parent = ctx
        return inst

    cdef const map[string, string]* _getMap(self):
        return &self._parent._ctx.getHeader().getWriteHeaders()


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

    @property
    def read_headers(self):
        if not self._readheaders:
            self._readheaders = ReadHeaders.create(self)
        return self._readheaders

    @property
    def write_headers(self):
        # So we don't create a cycle
        if not self._writeheaders:
            self._writeheaders = WriteHeaders.create(self)
        return self._writeheaders

    @property
    def priority(self):
        return Priority(<int>self._ctx.getCallPriority())

    def set_header(self, str key not None, str value not None):
        self._ctx.getHeader().setHeader(key.encode('utf-8'), value.encode('utf-8'))
