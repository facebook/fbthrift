from libcpp.memory cimport make_shared, static_pointer_cast, shared_ptr

from thrift.py3.client cimport Client, cTProcessorEventHandler
from thrift.py3 import get_client as _get_client
from thrift.py3.test.client_event_handler.handler cimport cTestClientEventHandler

cdef class TestHelper:
    cdef shared_ptr[cTestClientEventHandler] handler

    def __cinit__(self):
        self.handler = make_shared[cTestClientEventHandler]()

    def get_client(
        self,
        clientKlass,
        host='::1',
        int port=-1,
    ):
        cdef Client client = _get_client(clientKlass, host=host, port=port)
        client.add_event_handler(static_pointer_cast[cTProcessorEventHandler, cTestClientEventHandler](self.handler))
        return client

    def is_handler_called(self):
        return self.handler.get().preWriteCalled()

