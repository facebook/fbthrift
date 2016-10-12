from cpython.ref cimport PyObject
from thrift.lib.py3.thrift_server cimport (
  ServiceInterface,
  cTApplicationException
)
from folly_futures cimport cFollyPromise, cFollyUnit, c_unit
include "module_types.pxi"
include "module_promises.pxi"
include "module_callbacks.pxi"


from module_services_wrapper cimport cSimpleServiceInterface

cdef class SimpleServiceInterface(ServiceInterface):
    def __cinit__(self):
        self.interface_wrapper = cSimpleServiceInterface(<PyObject *> self)

    def __init__(self, loop=None):
        self.loop = loop or asyncio.get_event_loop()

    async def get_five(
            self):
        raise NotImplementedError("async def get_five is not implemented")


    async def add_five(
            self,
            num):
        raise NotImplementedError("async def add_five is not implemented")


    async def do_nothing(
            self):
        raise NotImplementedError("async def do_nothing is not implemented")


    async def concat(
            self,
            first,
            second):
        raise NotImplementedError("async def concat is not implemented")


    async def get_value(
            self,
            simple_struct):
        raise NotImplementedError("async def get_value is not implemented")


