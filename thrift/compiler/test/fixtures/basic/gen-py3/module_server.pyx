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
from module_types cimport (
    move,
    MyStruct,
    cMyStruct
)

include "module_promises.pxi"
include "module_callbacks.pxi"


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


