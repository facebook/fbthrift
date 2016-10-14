from libcpp.memory cimport shared_ptr, make_shared, unique_ptr, make_unique
from libcpp.string cimport string
from libcpp cimport bool as cbool
from cpython cimport bool as pbool
from libc.stdint cimport int8_t, int16_t, int32_t, int64_t
from cython.operator cimport dereference as deref
from cpython.ref cimport PyObject
from thrift.lib.py3.thrift_server cimport (
  ServiceInterface,
  cTApplicationException
)
from folly_futures cimport cFollyPromise, cFollyUnit, c_unit

import asyncio
import functools
import sys
import traceback

from module_types cimport (
    move,
    MyStruct,
    cMyStruct
)


cdef class Promise_void:
    cdef shared_ptr[cFollyPromise[cFollyUnit]] cPromise

    @staticmethod
    cdef create(shared_ptr[cFollyPromise[cFollyUnit]] cPromise):
        inst = <Promise_void>Promise_void.__new__(Promise_void)
        inst.cPromise = cPromise
        return inst

cdef class Promise_string:
    cdef shared_ptr[cFollyPromise[unique_ptr[string]]] cPromise

    @staticmethod
    cdef create(shared_ptr[cFollyPromise[unique_ptr[string]]] cPromise):
        inst = <Promise_string>Promise_string.__new__(Promise_string)
        inst.cPromise = cPromise
        return inst

cdef class Promise_bool:
    cdef shared_ptr[cFollyPromise[cbool]] cPromise

    @staticmethod
    cdef create(shared_ptr[cFollyPromise[cbool]] cPromise):
        inst = <Promise_bool>Promise_bool.__new__(Promise_bool)
        inst.cPromise = cPromise
        return inst

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


from module_services_wrapper cimport cMyServiceInterface
from module_services_wrapper cimport cMyServiceFastInterface

cdef class MyServiceInterface(ServiceInterface):
    def __cinit__(self):
        self.interface_wrapper = cMyServiceInterface(<PyObject *> self)

    def __init__(self, loop=None):
        self.loop = loop or asyncio.get_event_loop()

    async def ping(
            self):
        raise NotImplementedError("async def ping is not implemented")


    async def getRandomData(
            self):
        raise NotImplementedError("async def getRandomData is not implemented")


    async def hasDataById(
            self,
            id):
        raise NotImplementedError("async def hasDataById is not implemented")


    async def getDataById(
            self,
            id):
        raise NotImplementedError("async def getDataById is not implemented")


    async def putDataById(
            self,
            id,
            data):
        raise NotImplementedError("async def putDataById is not implemented")


    async def lobDataById(
            self,
            id,
            data):
        raise NotImplementedError("async def lobDataById is not implemented")


cdef class MyServiceFastInterface(ServiceInterface):
    def __cinit__(self):
        self.interface_wrapper = cMyServiceFastInterface(<PyObject *> self)

    def __init__(self, loop=None):
        self.loop = loop or asyncio.get_event_loop()

    async def ping(
            self):
        raise NotImplementedError("async def ping is not implemented")


    async def getRandomData(
            self):
        raise NotImplementedError("async def getRandomData is not implemented")


    async def hasDataById(
            self,
            id):
        raise NotImplementedError("async def hasDataById is not implemented")


    async def getDataById(
            self,
            id):
        raise NotImplementedError("async def getDataById is not implemented")


    async def putDataById(
            self,
            id,
            data):
        raise NotImplementedError("async def putDataById is not implemented")


    async def lobDataById(
            self,
            id,
            data):
        raise NotImplementedError("async def lobDataById is not implemented")


