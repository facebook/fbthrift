import asyncio
import functools
from libcpp.memory cimport shared_ptr, unique_ptr


cdef public void call_cy_MyService_ping(
    object self,
    shared_ptr[cFollyPromise[cFollyUnit]] cPromise
) with gil:
    promise = Promise_void.create(cPromise)
    # TODO: Wrap arguments in Python struct

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
    Promise_void  promise
):
    result = await self.ping()
    cResult = c_unit
    promise.cPromise.get().setValue(<cFollyUnit> cResult)

cdef public void call_cy_MyService_getRandomData(
    object self,
    shared_ptr[cFollyPromise[string]] cPromise
) with gil:
    promise = Promise_string.create(cPromise)
    # TODO: Wrap arguments in Python struct

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
    Promise_string  promise
):
    result = await self.getRandomData()
    cResult = 
    promise.cPromise.get().setValue(<string> cResult)

cdef public void call_cy_MyService_hasDataById(
    object self,
    shared_ptr[cFollyPromise[]] cPromise,
    int64_t id
) with gil:
    promise = Promise_bool.create(cPromise)
    # TODO: Wrap arguments in Python struct
    arg_id = id

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
    func = functools.partial(
        asyncio.ensure_future,
        MyService_hasDataById_coro(
            self,
            promise,
            arg_id
            ),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyService_hasDataById_coro(
    object self,
    Promise_bool  promise,
    int64_t id
):
    result = await self.hasDataById(id)
    cResult = 
    promise.cPromise.get().setValue(<> cResult)

cdef public void call_cy_MyService_getDataById(
    object self,
    shared_ptr[cFollyPromise[string]] cPromise,
    int64_t id
) with gil:
    promise = Promise_string.create(cPromise)
    # TODO: Wrap arguments in Python struct
    arg_id = id

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
    func = functools.partial(
        asyncio.ensure_future,
        MyService_getDataById_coro(
            self,
            promise,
            arg_id
            ),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyService_getDataById_coro(
    object self,
    Promise_string  promise,
    int64_t id
):
    result = await self.getDataById(id)
    cResult = 
    promise.cPromise.get().setValue(<string> cResult)

cdef public void call_cy_MyService_putDataById(
    object self,
    shared_ptr[cFollyPromise[cFollyUnit]] cPromise,
    int64_t id,
    string data
) with gil:
    promise = Promise_void.create(cPromise)
    # TODO: Wrap arguments in Python struct
    arg_id = id
    arg_data = data

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
    func = functools.partial(
        asyncio.ensure_future,
        MyService_putDataById_coro(
            self,
            promise,
            arg_id
            ,
            arg_data
            ),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyService_putDataById_coro(
    object self,
    Promise_void  promise,
    int64_t id,
    string data
):
    result = await self.putDataById(iddata)
    cResult = c_unit
    promise.cPromise.get().setValue(<cFollyUnit> cResult)

cdef public void call_cy_MyService_lobDataById(
    object self,
    shared_ptr[cFollyPromise[cFollyUnit]] cPromise,
    int64_t id,
    string data
) with gil:
    promise = Promise_void.create(cPromise)
    # TODO: Wrap arguments in Python struct
    arg_id = id
    arg_data = data

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
    func = functools.partial(
        asyncio.ensure_future,
        MyService_lobDataById_coro(
            self,
            promise,
            arg_id
            ,
            arg_data
            ),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyService_lobDataById_coro(
    object self,
    Promise_void  promise,
    int64_t id,
    string data
):
    result = await self.lobDataById(iddata)
    cResult = c_unit
    promise.cPromise.get().setValue(<cFollyUnit> cResult)

cdef public void call_cy_MyServiceFast_ping(
    object self,
    shared_ptr[cFollyPromise[cFollyUnit]] cPromise
) with gil:
    promise = Promise_void.create(cPromise)
    # TODO: Wrap arguments in Python struct

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
    Promise_void  promise
):
    result = await self.ping()
    cResult = c_unit
    promise.cPromise.get().setValue(<cFollyUnit> cResult)

cdef public void call_cy_MyServiceFast_getRandomData(
    object self,
    shared_ptr[cFollyPromise[string]] cPromise
) with gil:
    promise = Promise_string.create(cPromise)
    # TODO: Wrap arguments in Python struct

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
    Promise_string  promise
):
    result = await self.getRandomData()
    cResult = 
    promise.cPromise.get().setValue(<string> cResult)

cdef public void call_cy_MyServiceFast_hasDataById(
    object self,
    shared_ptr[cFollyPromise[]] cPromise,
    int64_t id
) with gil:
    promise = Promise_bool.create(cPromise)
    # TODO: Wrap arguments in Python struct
    arg_id = id

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
    func = functools.partial(
        asyncio.ensure_future,
        MyServiceFast_hasDataById_coro(
            self,
            promise,
            arg_id
            ),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyServiceFast_hasDataById_coro(
    object self,
    Promise_bool  promise,
    int64_t id
):
    result = await self.hasDataById(id)
    cResult = 
    promise.cPromise.get().setValue(<> cResult)

cdef public void call_cy_MyServiceFast_getDataById(
    object self,
    shared_ptr[cFollyPromise[string]] cPromise,
    int64_t id
) with gil:
    promise = Promise_string.create(cPromise)
    # TODO: Wrap arguments in Python struct
    arg_id = id

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
    func = functools.partial(
        asyncio.ensure_future,
        MyServiceFast_getDataById_coro(
            self,
            promise,
            arg_id
            ),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyServiceFast_getDataById_coro(
    object self,
    Promise_string  promise,
    int64_t id
):
    result = await self.getDataById(id)
    cResult = 
    promise.cPromise.get().setValue(<string> cResult)

cdef public void call_cy_MyServiceFast_putDataById(
    object self,
    shared_ptr[cFollyPromise[cFollyUnit]] cPromise,
    int64_t id,
    string data
) with gil:
    promise = Promise_void.create(cPromise)
    # TODO: Wrap arguments in Python struct
    arg_id = id
    arg_data = data

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
    func = functools.partial(
        asyncio.ensure_future,
        MyServiceFast_putDataById_coro(
            self,
            promise,
            arg_id
            ,
            arg_data
            ),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyServiceFast_putDataById_coro(
    object self,
    Promise_void  promise,
    int64_t id,
    string data
):
    result = await self.putDataById(iddata)
    cResult = c_unit
    promise.cPromise.get().setValue(<cFollyUnit> cResult)

cdef public void call_cy_MyServiceFast_lobDataById(
    object self,
    shared_ptr[cFollyPromise[cFollyUnit]] cPromise,
    int64_t id,
    string data
) with gil:
    promise = Promise_void.create(cPromise)
    # TODO: Wrap arguments in Python struct
    arg_id = id
    arg_data = data

    #TODO: asyncio.run_coroutine_threadsafe... after Python 3.5.2 lands
    func = functools.partial(
        asyncio.ensure_future,
        MyServiceFast_lobDataById_coro(
            self,
            promise,
            arg_id
            ,
            arg_data
            ),
        loop=self.loop)
    self.loop.call_soon_threadsafe(func)

async def MyServiceFast_lobDataById_coro(
    object self,
    Promise_void  promise,
    int64_t id,
    string data
):
    result = await self.lobDataById(iddata)
    cResult = c_unit
    promise.cPromise.get().setValue(<cFollyUnit> cResult)

