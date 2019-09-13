from folly.executor cimport get_executor

import asyncio


cdef class SemiStream:
    """
    Base class for all SemiStream object
    """
    def __cinit__(SemiStream self):
        self._executor = get_executor()

    def __aiter__(self):
        return self

    def __anext__(self):
        loop = asyncio.get_event_loop()
        future = loop.create_future()
        future.set_exception(RuntimeError("Not implemented"))
        return future

cdef class ResponseAndSemiStream:
    def get_response(self):
        pass

    def get_stream(self, int buffer_size):
        pass


