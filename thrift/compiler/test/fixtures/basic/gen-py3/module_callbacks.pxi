import asyncio
import functools
import sys
import traceback
from libcpp.memory cimport shared_ptr, unique_ptr
from cython.operator import dereference as deref


cdef public void call_cy_MyService_ping(
    object self,
    shared_ptr[cFollyPromise[cFollyUnit]] cPromise
) with gil:
    promise = Promise_void.create(cPromise)

    func = functools.partial(
        asyncio.ensure_future,
        MyService_ping_coro(
            self,
            promise),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyService_ping_coro(
    object self,
    Promise_void promise
):
    try:
      result = await self.ping()
    except Exception as ex:
        print(
            "Unexpected error in service handler ping:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(c_unit)

cdef public void call_cy_MyService_getRandomData(
    object self,
    shared_ptr[cFollyPromise[unique_ptr[string]]] cPromise
) with gil:
    promise = Promise_string.create(cPromise)

    func = functools.partial(
        asyncio.ensure_future,
        MyService_getRandomData_coro(
            self,
            promise),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyService_getRandomData_coro(
    object self,
    Promise_string promise
):
    try:
      result = await self.getRandomData()
    except Exception as ex:
        print(
            "Unexpected error in service handler getRandomData:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(make_unique[string](<string> result.encode('UTF-8')))

cdef public void call_cy_MyService_hasDataById(
    object self,
    shared_ptr[cFollyPromise[cbool]] cPromise,
    int64_t id
) with gil:
    promise = Promise_bool.create(cPromise)
    arg_id = id

    func = functools.partial(
        asyncio.ensure_future,
        MyService_hasDataById_coro(
            self,
            promise,
            arg_id),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyService_hasDataById_coro(
    object self,
    Promise_bool promise,
    id
):
    try:
      result = await self.hasDataById(
          id)
    except Exception as ex:
        print(
            "Unexpected error in service handler hasDataById:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(<cbool> result)

cdef public void call_cy_MyService_getDataById(
    object self,
    shared_ptr[cFollyPromise[unique_ptr[string]]] cPromise,
    int64_t id
) with gil:
    promise = Promise_string.create(cPromise)
    arg_id = id

    func = functools.partial(
        asyncio.ensure_future,
        MyService_getDataById_coro(
            self,
            promise,
            arg_id),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyService_getDataById_coro(
    object self,
    Promise_string promise,
    id
):
    try:
      result = await self.getDataById(
          id)
    except Exception as ex:
        print(
            "Unexpected error in service handler getDataById:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(make_unique[string](<string> result.encode('UTF-8')))

cdef public void call_cy_MyService_putDataById(
    object self,
    shared_ptr[cFollyPromise[cFollyUnit]] cPromise,
    int64_t id,
    unique_ptr[string] data
) with gil:
    promise = Promise_void.create(cPromise)
    arg_id = id
    arg_data = (deref(data.get())).decode()

    func = functools.partial(
        asyncio.ensure_future,
        MyService_putDataById_coro(
            self,
            promise,
            arg_id,
            arg_data),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyService_putDataById_coro(
    object self,
    Promise_void promise,
    id,
    data
):
    try:
      result = await self.putDataById(
          id,
          data)
    except Exception as ex:
        print(
            "Unexpected error in service handler putDataById:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(c_unit)

cdef public void call_cy_MyService_lobDataById(
    object self,
    shared_ptr[cFollyPromise[cFollyUnit]] cPromise,
    int64_t id,
    unique_ptr[string] data
) with gil:
    promise = Promise_void.create(cPromise)
    arg_id = id
    arg_data = (deref(data.get())).decode()

    func = functools.partial(
        asyncio.ensure_future,
        MyService_lobDataById_coro(
            self,
            promise,
            arg_id,
            arg_data),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyService_lobDataById_coro(
    object self,
    Promise_void promise,
    id,
    data
):
    try:
      result = await self.lobDataById(
          id,
          data)
    except Exception as ex:
        print(
            "Unexpected error in service handler lobDataById:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(c_unit)

cdef public void call_cy_MyServiceFast_ping(
    object self,
    shared_ptr[cFollyPromise[cFollyUnit]] cPromise
) with gil:
    promise = Promise_void.create(cPromise)

    func = functools.partial(
        asyncio.ensure_future,
        MyServiceFast_ping_coro(
            self,
            promise),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyServiceFast_ping_coro(
    object self,
    Promise_void promise
):
    try:
      result = await self.ping()
    except Exception as ex:
        print(
            "Unexpected error in service handler ping:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(c_unit)

cdef public void call_cy_MyServiceFast_getRandomData(
    object self,
    shared_ptr[cFollyPromise[unique_ptr[string]]] cPromise
) with gil:
    promise = Promise_string.create(cPromise)

    func = functools.partial(
        asyncio.ensure_future,
        MyServiceFast_getRandomData_coro(
            self,
            promise),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyServiceFast_getRandomData_coro(
    object self,
    Promise_string promise
):
    try:
      result = await self.getRandomData()
    except Exception as ex:
        print(
            "Unexpected error in service handler getRandomData:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(make_unique[string](<string> result.encode('UTF-8')))

cdef public void call_cy_MyServiceFast_hasDataById(
    object self,
    shared_ptr[cFollyPromise[cbool]] cPromise,
    int64_t id
) with gil:
    promise = Promise_bool.create(cPromise)
    arg_id = id

    func = functools.partial(
        asyncio.ensure_future,
        MyServiceFast_hasDataById_coro(
            self,
            promise,
            arg_id),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyServiceFast_hasDataById_coro(
    object self,
    Promise_bool promise,
    id
):
    try:
      result = await self.hasDataById(
          id)
    except Exception as ex:
        print(
            "Unexpected error in service handler hasDataById:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(<cbool> result)

cdef public void call_cy_MyServiceFast_getDataById(
    object self,
    shared_ptr[cFollyPromise[unique_ptr[string]]] cPromise,
    int64_t id
) with gil:
    promise = Promise_string.create(cPromise)
    arg_id = id

    func = functools.partial(
        asyncio.ensure_future,
        MyServiceFast_getDataById_coro(
            self,
            promise,
            arg_id),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyServiceFast_getDataById_coro(
    object self,
    Promise_string promise,
    id
):
    try:
      result = await self.getDataById(
          id)
    except Exception as ex:
        print(
            "Unexpected error in service handler getDataById:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(make_unique[string](<string> result.encode('UTF-8')))

cdef public void call_cy_MyServiceFast_putDataById(
    object self,
    shared_ptr[cFollyPromise[cFollyUnit]] cPromise,
    int64_t id,
    unique_ptr[string] data
) with gil:
    promise = Promise_void.create(cPromise)
    arg_id = id
    arg_data = (deref(data.get())).decode()

    func = functools.partial(
        asyncio.ensure_future,
        MyServiceFast_putDataById_coro(
            self,
            promise,
            arg_id,
            arg_data),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyServiceFast_putDataById_coro(
    object self,
    Promise_void promise,
    id,
    data
):
    try:
      result = await self.putDataById(
          id,
          data)
    except Exception as ex:
        print(
            "Unexpected error in service handler putDataById:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(c_unit)

cdef public void call_cy_MyServiceFast_lobDataById(
    object self,
    shared_ptr[cFollyPromise[cFollyUnit]] cPromise,
    int64_t id,
    unique_ptr[string] data
) with gil:
    promise = Promise_void.create(cPromise)
    arg_id = id
    arg_data = (deref(data.get())).decode()

    func = functools.partial(
        asyncio.ensure_future,
        MyServiceFast_lobDataById_coro(
            self,
            promise,
            arg_id,
            arg_data),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyServiceFast_lobDataById_coro(
    object self,
    Promise_void promise,
    id,
    data
):
    try:
      result = await self.lobDataById(
          id,
          data)
    except Exception as ex:
        print(
            "Unexpected error in service handler lobDataById:",
            file=sys.stderr)
        traceback.print_exc()
        deref(promise.cPromise).setException(cTApplicationException(
            repr(ex).encode('UTF-8')
        ))
    else:
        deref(promise.cPromise).setValue(c_unit)

