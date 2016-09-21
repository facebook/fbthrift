import asyncio
import functools
from libcpp.memory cimport shared_ptr, unique_ptr
from cython.operator import dereference as deref


cdef public void call_cy_MyService_ping(
    object self,
    shared_ptr[cFollyPromise[cFollyUnit]] cPromise
) with gil:
    promise = Promise_void.create(cPromise)

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
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
    result = await self.ping()
    deref(promise.cPromise).setValue(c_unit)

cdef public void call_cy_MyService_getRandomData(
    object self,
    shared_ptr[cFollyPromise[unique_ptr[string]]] cPromise
) with gil:
    promise = Promise_string.create(cPromise)

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
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
    result = await self.getRandomData()
    deref(promise.cPromise).setValue(make_unique[string](<string> result.encode('UTF-8')))

cdef public void call_cy_MyService_hasDataById(
    object self,
    shared_ptr[cFollyPromise[]] cPromise,
    int64_t id
) with gil:
    promise = Promise_bool.create(cPromise)
    arg_id = id

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
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
    result = await self.hasDataById(
        id)
    deref(promise.cPromise).setValue()

cdef public void call_cy_MyService_getDataById(
    object self,
    shared_ptr[cFollyPromise[unique_ptr[string]]] cPromise,
    int64_t id
) with gil:
    promise = Promise_string.create(cPromise)
    arg_id = id

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
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
    result = await self.getDataById(
        id)
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

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
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
    result = await self.putDataById(
        id,
        data)
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

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
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
    result = await self.lobDataById(
        id,
        data)
    deref(promise.cPromise).setValue(c_unit)

cdef public void call_cy_MyServiceFast_ping(
    object self,
    shared_ptr[cFollyPromise[cFollyUnit]] cPromise
) with gil:
    promise = Promise_void.create(cPromise)

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
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
    result = await self.ping()
    deref(promise.cPromise).setValue(c_unit)

cdef public void call_cy_MyServiceFast_getRandomData(
    object self,
    shared_ptr[cFollyPromise[unique_ptr[string]]] cPromise
) with gil:
    promise = Promise_string.create(cPromise)

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
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
    result = await self.getRandomData()
    deref(promise.cPromise).setValue(make_unique[string](<string> result.encode('UTF-8')))

cdef public void call_cy_MyServiceFast_hasDataById(
    object self,
    shared_ptr[cFollyPromise[]] cPromise,
    int64_t id
) with gil:
    promise = Promise_bool.create(cPromise)
    arg_id = id

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
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
    result = await self.hasDataById(
        id)
    deref(promise.cPromise).setValue()

cdef public void call_cy_MyServiceFast_getDataById(
    object self,
    shared_ptr[cFollyPromise[unique_ptr[string]]] cPromise,
    int64_t id
) with gil:
    promise = Promise_string.create(cPromise)
    arg_id = id

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
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
    result = await self.getDataById(
        id)
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

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
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
    result = await self.putDataById(
        id,
        data)
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

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
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
    result = await self.lobDataById(
        id,
        data)
    deref(promise.cPromise).setValue(c_unit)

