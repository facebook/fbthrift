from libcpp.memory cimport make_unique
from thrift.py3.server cimport (
    cSSLPolicy, SSLPolicy__DISABLED, SSLPolicy__PERMITTED, SSLPolicy__REQUIRED
)

import asyncio
from enum import Enum

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
