cimport cython
from libcpp.memory cimport unique_ptr, make_unique, shared_ptr
from folly_futures cimport cFollyPromise, cFollyUnit, c_unit

import asyncio


cdef class ThriftServer:
    cdef unique_ptr[cThriftServer] server
    cdef ServiceInterface handler
    cdef object loop

    def __cinit__(self):
        self.server = make_unique[cThriftServer]()


    def __init__(self, ServiceInterface handler, port, loop=None):
        self.loop = loop or asyncio.get_event_loop()
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

    def stop(self):
        self.server.get().stop()
