from cpython.ref cimport PyObject
include "thriftlib/py3_thrift_server.pxi"
include "module_types.pxi"
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


