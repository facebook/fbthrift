from cython.operator cimport dereference as deref
from libcpp.memory cimport shared_ptr, make_shared
from thrift.lib.py3.folly cimport cFollyEventBase

from concurrent.futures import ThreadPoolExecutor

import asyncio

cdef class EventBase:
    def __cinit__(self, loop, executor):
        self._folly_event_base = make_shared[cFollyEventBase]()
        self.asyncio_loop = loop
        self.executor = executor
        loop.run_in_executor(executor, self._loop_forever)

    def _loop(self):
        with nogil:
            deref(self._folly_event_base).loop()

    def _loop_forever(self):
        with nogil:
            deref(self._folly_event_base).loopForever()

    def close(self):
        with nogil:
            deref(self._folly_event_base).terminateLoopSoon()
        return self.asyncio_loop.run_in_executor(
            self.executor, self._loop)

def get_event_base(loop=None):
    if loop is None:
        loop = asyncio.get_event_loop()
    if not hasattr(loop, '_event_base_thread'):
        event_base_thread = ThreadPoolExecutor(max_workers=1)
        event_base_thread._event_base = loop.run_in_executor(
            event_base_thread,
            EventBase,
            loop,
            event_base_thread)
        loop._event_base_thread = event_base_thread
    return loop._event_base_thread._event_base

async def close_event_base(loop=None):
    eb = await get_event_base(loop)
    await eb.close()
