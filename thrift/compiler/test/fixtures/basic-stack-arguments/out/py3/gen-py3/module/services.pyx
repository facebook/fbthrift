#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/basic-stack-arguments/src/module.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

cimport cython
from typing import AsyncIterator
from cpython.version cimport PY_VERSION_HEX
from libc.stdint cimport (
    int8_t as cint8_t,
    int16_t as cint16_t,
    int32_t as cint32_t,
    int64_t as cint64_t,
)
from libcpp.memory cimport shared_ptr, make_shared, unique_ptr
from libcpp.string cimport string
from libcpp cimport bool as cbool
from cpython cimport bool as pbool
from libcpp.vector cimport vector
from libcpp.set cimport set as cset
from libcpp.map cimport map as cmap
from libcpp.utility cimport move as cmove
from libcpp.pair cimport pair
from cython.operator cimport dereference as deref
from cpython.ref cimport PyObject
from thrift.python.exceptions cimport (
    ApplicationError as __ApplicationError,
    cTApplicationException,
    cTApplicationExceptionType__UNKNOWN,
)
from thrift.py3.server cimport ServiceInterface
from thrift.python.server_impl.request_context cimport (
    Cpp2RequestContext,
    RequestContext,
    THRIFT_REQUEST_CONTEXT as __THRIFT_REQUEST_CONTEXT,
)
from thrift.python.server_impl.request_context import RequestContext
from folly cimport (
  cFollyPromise,
  cFollyUnit,
  c_unit,
)
from thrift.python.common cimport (
    cThriftServiceMetadataResponse as __fbthrift_cThriftServiceMetadataResponse,
    ServiceMetadata,
    MetadataBox as __MetadataBox,
)

from thrift.py3.types cimport make_unique

cimport folly.futures
from folly.executor cimport get_executor
cimport folly.iobuf as _fbthrift_iobuf
import folly.iobuf as _fbthrift_iobuf
from folly.iobuf cimport move as move_iobuf
from folly.memory cimport to_shared_ptr as __to_shared_ptr

cimport module.types as _module_types
cimport module.cbindings as _module_cbindings
import module.types as _module_types

cimport module.services_interface as _fbthrift_services_interface

import asyncio
import functools
import sys
import traceback
import types as _py_types

from module.services_wrapper cimport cMyServiceInterface
from module.services_wrapper cimport cMyServiceFastInterface
from module.services_wrapper cimport cDbMixedStackArgumentsInterface



@cython.auto_pickle(False)
cdef class Promise_binary:
    cdef cFollyPromise[unique_ptr[string]]* cPromise

    def __cinit__(self):
        self.cPromise = new cFollyPromise[unique_ptr[string]](cFollyPromise[unique_ptr[string]].makeEmpty())

    def __dealloc__(self):
        del self.cPromise

    @staticmethod
    cdef _fbthrift_create(cFollyPromise[unique_ptr[string]] cPromise):
        cdef Promise_binary inst = Promise_binary.__new__(Promise_binary)
        inst.cPromise[0] = cmove(cPromise)
        return inst

@cython.auto_pickle(False)
cdef class Promise__sa_binary:
    cdef cFollyPromise[string]* cPromise

    def __cinit__(self):
        self.cPromise = new cFollyPromise[string](cFollyPromise[string].makeEmpty())

    def __dealloc__(self):
        del self.cPromise

    @staticmethod
    cdef _fbthrift_create(cFollyPromise[string] cPromise):
        cdef Promise__sa_binary inst = Promise__sa_binary.__new__(Promise__sa_binary)
        inst.cPromise[0] = cmove(cPromise)
        return inst

@cython.auto_pickle(False)
cdef class Promise__sa_cbool:
    cdef cFollyPromise[cbool]* cPromise

    def __cinit__(self):
        self.cPromise = new cFollyPromise[cbool](cFollyPromise[cbool].makeEmpty())

    def __dealloc__(self):
        del self.cPromise

    @staticmethod
    cdef _fbthrift_create(cFollyPromise[cbool] cPromise):
        cdef Promise__sa_cbool inst = Promise__sa_cbool.__new__(Promise__sa_cbool)
        inst.cPromise[0] = cmove(cPromise)
        return inst

@cython.auto_pickle(False)
cdef class Promise__sa_string:
    cdef cFollyPromise[string]* cPromise

    def __cinit__(self):
        self.cPromise = new cFollyPromise[string](cFollyPromise[string].makeEmpty())

    def __dealloc__(self):
        del self.cPromise

    @staticmethod
    cdef _fbthrift_create(cFollyPromise[string] cPromise):
        cdef Promise__sa_string inst = Promise__sa_string.__new__(Promise__sa_string)
        inst.cPromise[0] = cmove(cPromise)
        return inst

@cython.auto_pickle(False)
cdef class Promise__sa_cFollyUnit:
    cdef cFollyPromise[cFollyUnit]* cPromise

    def __cinit__(self):
        self.cPromise = new cFollyPromise[cFollyUnit](cFollyPromise[cFollyUnit].makeEmpty())

    def __dealloc__(self):
        del self.cPromise

    @staticmethod
    cdef _fbthrift_create(cFollyPromise[cFollyUnit] cPromise):
        cdef Promise__sa_cFollyUnit inst = Promise__sa_cFollyUnit.__new__(Promise__sa_cFollyUnit)
        inst.cPromise[0] = cmove(cPromise)
        return inst

cdef object _MyService_annotations = _py_types.MappingProxyType({
})


@cython.auto_pickle(False)
cdef class MyServiceInterface(
    ServiceInterface
):
    annotations = _MyService_annotations

    def __cinit__(self):
        self._cpp_obj = cMyServiceInterface(
            <PyObject *> self,
            get_executor()
        )

    _fbthrift_annotations_DO_NOT_USE_hasDataById = {
        'return': 'bool',
        'id': 'int', 
    }

    async def hasDataById(
            self,
            id):
        raise NotImplementedError("async def hasDataById is not implemented")

    _fbthrift_annotations_DO_NOT_USE_getDataById = {
        'return': 'str',
        'id': 'int', 
    }

    async def getDataById(
            self,
            id):
        raise NotImplementedError("async def getDataById is not implemented")

    _fbthrift_annotations_DO_NOT_USE_putDataById = {
        'return': 'None',
        'id': 'int', 'data': 'str', 
    }

    async def putDataById(
            self,
            id,
            data):
        raise NotImplementedError("async def putDataById is not implemented")

    _fbthrift_annotations_DO_NOT_USE_lobDataById = {
        'return': 'None',
        'id': 'int', 'data': 'str', 
    }

    async def lobDataById(
            self,
            id,
            data):
        raise NotImplementedError("async def lobDataById is not implemented")

    @staticmethod
    def __get_metadata__():
        cdef __fbthrift_cThriftServiceMetadataResponse response
        ServiceMetadata[_fbthrift_services_interface.cMyServiceSvIf].gen(response)
        return __MetadataBox.box(cmove(deref(response.metadata_ref())))

    @staticmethod
    def __get_thrift_name__():
        return "module.MyService"

cdef object _MyServiceFast_annotations = _py_types.MappingProxyType({
})


@cython.auto_pickle(False)
cdef class MyServiceFastInterface(
    ServiceInterface
):
    annotations = _MyServiceFast_annotations

    def __cinit__(self):
        self._cpp_obj = cMyServiceFastInterface(
            <PyObject *> self,
            get_executor()
        )

    _fbthrift_annotations_DO_NOT_USE_hasDataById = {
        'return': 'bool',
        'id': 'int', 
    }

    async def hasDataById(
            self,
            id):
        raise NotImplementedError("async def hasDataById is not implemented")

    _fbthrift_annotations_DO_NOT_USE_getDataById = {
        'return': 'str',
        'id': 'int', 
    }

    async def getDataById(
            self,
            id):
        raise NotImplementedError("async def getDataById is not implemented")

    _fbthrift_annotations_DO_NOT_USE_putDataById = {
        'return': 'None',
        'id': 'int', 'data': 'str', 
    }

    async def putDataById(
            self,
            id,
            data):
        raise NotImplementedError("async def putDataById is not implemented")

    _fbthrift_annotations_DO_NOT_USE_lobDataById = {
        'return': 'None',
        'id': 'int', 'data': 'str', 
    }

    async def lobDataById(
            self,
            id,
            data):
        raise NotImplementedError("async def lobDataById is not implemented")

    @staticmethod
    def __get_metadata__():
        cdef __fbthrift_cThriftServiceMetadataResponse response
        ServiceMetadata[_fbthrift_services_interface.cMyServiceFastSvIf].gen(response)
        return __MetadataBox.box(cmove(deref(response.metadata_ref())))

    @staticmethod
    def __get_thrift_name__():
        return "module.MyServiceFast"

cdef object _DbMixedStackArguments_annotations = _py_types.MappingProxyType({
})


@cython.auto_pickle(False)
cdef class DbMixedStackArgumentsInterface(
    ServiceInterface
):
    annotations = _DbMixedStackArguments_annotations

    def __cinit__(self):
        self._cpp_obj = cDbMixedStackArgumentsInterface(
            <PyObject *> self,
            get_executor()
        )

    _fbthrift_annotations_DO_NOT_USE_getDataByKey0 = {
        'return': 'bytes',
        'key': 'str', 
    }

    async def getDataByKey0(
            self,
            key):
        raise NotImplementedError("async def getDataByKey0 is not implemented")

    _fbthrift_annotations_DO_NOT_USE_getDataByKey1 = {
        'return': 'bytes',
        'key': 'str', 
    }

    async def getDataByKey1(
            self,
            key):
        raise NotImplementedError("async def getDataByKey1 is not implemented")

    @staticmethod
    def __get_metadata__():
        cdef __fbthrift_cThriftServiceMetadataResponse response
        ServiceMetadata[_fbthrift_services_interface.cDbMixedStackArgumentsSvIf].gen(response)
        return __MetadataBox.box(cmove(deref(response.metadata_ref())))

    @staticmethod
    def __get_thrift_name__():
        return "module.DbMixedStackArguments"



cdef api void call_cy_MyService_hasDataById(
    object self,
    Cpp2RequestContext* ctx,
    cFollyPromise[cbool] cPromise,
    cint64_t id
) noexcept:
    cdef Promise__sa_cbool __promise = Promise__sa_cbool._fbthrift_create(cmove(cPromise))
    arg_id = id
    __context = RequestContext._fbthrift_create(ctx)
    __context_token = __THRIFT_REQUEST_CONTEXT.set(__context)
    asyncio.get_event_loop().create_task(
        MyService_hasDataById_coro(
            self,
            __promise,
            arg_id
        )
    )
    __THRIFT_REQUEST_CONTEXT.reset(__context_token)
cdef api void call_cy_MyService_getDataById(
    object self,
    Cpp2RequestContext* ctx,
    cFollyPromise[string] cPromise,
    cint64_t id
) noexcept:
    cdef Promise__sa_string __promise = Promise__sa_string._fbthrift_create(cmove(cPromise))
    arg_id = id
    __context = RequestContext._fbthrift_create(ctx)
    __context_token = __THRIFT_REQUEST_CONTEXT.set(__context)
    asyncio.get_event_loop().create_task(
        MyService_getDataById_coro(
            self,
            __promise,
            arg_id
        )
    )
    __THRIFT_REQUEST_CONTEXT.reset(__context_token)
cdef api void call_cy_MyService_putDataById(
    object self,
    Cpp2RequestContext* ctx,
    cFollyPromise[cFollyUnit] cPromise,
    cint64_t id,
    string data
) noexcept:
    cdef Promise__sa_cFollyUnit __promise = Promise__sa_cFollyUnit._fbthrift_create(cmove(cPromise))
    arg_id = id
    arg_data = data.data().decode('UTF-8')
    __context = RequestContext._fbthrift_create(ctx)
    __context_token = __THRIFT_REQUEST_CONTEXT.set(__context)
    asyncio.get_event_loop().create_task(
        MyService_putDataById_coro(
            self,
            __promise,
            arg_id,
            arg_data
        )
    )
    __THRIFT_REQUEST_CONTEXT.reset(__context_token)
cdef api void call_cy_MyService_lobDataById(
    object self,
    Cpp2RequestContext* ctx,
    cFollyPromise[cFollyUnit] cPromise,
    cint64_t id,
    string data
) noexcept:
    cdef Promise__sa_cFollyUnit __promise = Promise__sa_cFollyUnit._fbthrift_create(cmove(cPromise))
    arg_id = id
    arg_data = data.data().decode('UTF-8')
    __context = RequestContext._fbthrift_create(ctx)
    __context_token = __THRIFT_REQUEST_CONTEXT.set(__context)
    asyncio.get_event_loop().create_task(
        MyService_lobDataById_coro(
            self,
            __promise,
            arg_id,
            arg_data
        )
    )
    __THRIFT_REQUEST_CONTEXT.reset(__context_token)
cdef api void call_cy_MyService_onStartServing(
    object self,
    cFollyPromise[cFollyUnit] cPromise
) noexcept:
    cdef Promise__sa_cFollyUnit __promise = Promise__sa_cFollyUnit._fbthrift_create(cmove(cPromise))
    asyncio.get_event_loop().create_task(
        MyService_onStartServing_coro(
            self,
            __promise
        )
    )
cdef api void call_cy_MyService_onStopRequested(
    object self,
    cFollyPromise[cFollyUnit] cPromise
) noexcept:
    cdef Promise__sa_cFollyUnit __promise = Promise__sa_cFollyUnit._fbthrift_create(cmove(cPromise))
    asyncio.get_event_loop().create_task(
        MyService_onStopRequested_coro(
            self,
            __promise
        )
    )
async def MyService_hasDataById_coro(
    object self,
    Promise__sa_cbool promise,
    id
):
    try:
        result = await self.hasDataById(
                    id)
    except __ApplicationError as ex:
        # If the handler raised an ApplicationError convert it to a C++ one
        promise.cPromise.setException(cTApplicationException(
            ex.type.value, ex.message.encode('UTF-8')
        ))
    except Exception as ex:
        print(
            "Unexpected error in service handler MyService.hasDataById:",
            file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, repr(ex).encode('UTF-8')
        ))
    except asyncio.CancelledError as ex:
        print("Coroutine was cancelled in service handler MyService.hasDataById:", file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, (f'Application was cancelled on the server with message: {str(ex)}').encode('UTF-8')
        ))
    else:
        promise.cPromise.setValue(<cbool> result)

async def MyService_getDataById_coro(
    object self,
    Promise__sa_string promise,
    id
):
    try:
        result = await self.getDataById(
                    id)
    except __ApplicationError as ex:
        # If the handler raised an ApplicationError convert it to a C++ one
        promise.cPromise.setException(cTApplicationException(
            ex.type.value, ex.message.encode('UTF-8')
        ))
    except Exception as ex:
        print(
            "Unexpected error in service handler MyService.getDataById:",
            file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, repr(ex).encode('UTF-8')
        ))
    except asyncio.CancelledError as ex:
        print("Coroutine was cancelled in service handler MyService.getDataById:", file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, (f'Application was cancelled on the server with message: {str(ex)}').encode('UTF-8')
        ))
    else:
        promise.cPromise.setValue(<string?> result.encode('UTF-8'))

async def MyService_putDataById_coro(
    object self,
    Promise__sa_cFollyUnit promise,
    id,
    data
):
    try:
        result = await self.putDataById(
                    id,
                    data)
    except __ApplicationError as ex:
        # If the handler raised an ApplicationError convert it to a C++ one
        promise.cPromise.setException(cTApplicationException(
            ex.type.value, ex.message.encode('UTF-8')
        ))
    except Exception as ex:
        print(
            "Unexpected error in service handler MyService.putDataById:",
            file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, repr(ex).encode('UTF-8')
        ))
    except asyncio.CancelledError as ex:
        print("Coroutine was cancelled in service handler MyService.putDataById:", file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, (f'Application was cancelled on the server with message: {str(ex)}').encode('UTF-8')
        ))
    else:
        promise.cPromise.setValue(c_unit)

async def MyService_lobDataById_coro(
    object self,
    Promise__sa_cFollyUnit promise,
    id,
    data
):
    try:
        result = await self.lobDataById(
                    id,
                    data)
    except __ApplicationError as ex:
        # If the handler raised an ApplicationError convert it to a C++ one
        promise.cPromise.setException(cTApplicationException(
            ex.type.value, ex.message.encode('UTF-8')
        ))
    except Exception as ex:
        print(
            "Unexpected error in service handler MyService.lobDataById:",
            file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, repr(ex).encode('UTF-8')
        ))
    except asyncio.CancelledError as ex:
        print("Coroutine was cancelled in service handler MyService.lobDataById:", file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, (f'Application was cancelled on the server with message: {str(ex)}').encode('UTF-8')
        ))
    else:
        promise.cPromise.setValue(c_unit)

async def MyService_onStartServing_coro(
    object self,
    Promise__sa_cFollyUnit promise
):
    try:
        result = await self.onStartServing()
    except __ApplicationError as ex:
        # If the handler raised an ApplicationError convert it to a C++ one
        promise.cPromise.setException(cTApplicationException(
            ex.type.value, ex.message.encode('UTF-8')
        ))
    except Exception as ex:
        print(
            "Unexpected error in service handler MyService.onStartServing:",
            file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, repr(ex).encode('UTF-8')
        ))
    except asyncio.CancelledError as ex:
        print("Coroutine was cancelled in service handler MyService.onStartServing:", file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, (f'Application was cancelled on the server with message: {str(ex)}').encode('UTF-8')
        ))
    else:
        promise.cPromise.setValue(c_unit)

async def MyService_onStopRequested_coro(
    object self,
    Promise__sa_cFollyUnit promise
):
    try:
        result = await self.onStopRequested()
    except __ApplicationError as ex:
        # If the handler raised an ApplicationError convert it to a C++ one
        promise.cPromise.setException(cTApplicationException(
            ex.type.value, ex.message.encode('UTF-8')
        ))
    except Exception as ex:
        print(
            "Unexpected error in service handler MyService.onStopRequested:",
            file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, repr(ex).encode('UTF-8')
        ))
    except asyncio.CancelledError as ex:
        print("Coroutine was cancelled in service handler MyService.onStopRequested:", file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, (f'Application was cancelled on the server with message: {str(ex)}').encode('UTF-8')
        ))
    else:
        promise.cPromise.setValue(c_unit)

cdef api void call_cy_MyServiceFast_hasDataById(
    object self,
    Cpp2RequestContext* ctx,
    cFollyPromise[cbool] cPromise,
    cint64_t id
) noexcept:
    cdef Promise__sa_cbool __promise = Promise__sa_cbool._fbthrift_create(cmove(cPromise))
    arg_id = id
    __context = RequestContext._fbthrift_create(ctx)
    __context_token = __THRIFT_REQUEST_CONTEXT.set(__context)
    asyncio.get_event_loop().create_task(
        MyServiceFast_hasDataById_coro(
            self,
            __promise,
            arg_id
        )
    )
    __THRIFT_REQUEST_CONTEXT.reset(__context_token)
cdef api void call_cy_MyServiceFast_getDataById(
    object self,
    Cpp2RequestContext* ctx,
    cFollyPromise[string] cPromise,
    cint64_t id
) noexcept:
    cdef Promise__sa_string __promise = Promise__sa_string._fbthrift_create(cmove(cPromise))
    arg_id = id
    __context = RequestContext._fbthrift_create(ctx)
    __context_token = __THRIFT_REQUEST_CONTEXT.set(__context)
    asyncio.get_event_loop().create_task(
        MyServiceFast_getDataById_coro(
            self,
            __promise,
            arg_id
        )
    )
    __THRIFT_REQUEST_CONTEXT.reset(__context_token)
cdef api void call_cy_MyServiceFast_putDataById(
    object self,
    Cpp2RequestContext* ctx,
    cFollyPromise[cFollyUnit] cPromise,
    cint64_t id,
    string data
) noexcept:
    cdef Promise__sa_cFollyUnit __promise = Promise__sa_cFollyUnit._fbthrift_create(cmove(cPromise))
    arg_id = id
    arg_data = data.data().decode('UTF-8')
    __context = RequestContext._fbthrift_create(ctx)
    __context_token = __THRIFT_REQUEST_CONTEXT.set(__context)
    asyncio.get_event_loop().create_task(
        MyServiceFast_putDataById_coro(
            self,
            __promise,
            arg_id,
            arg_data
        )
    )
    __THRIFT_REQUEST_CONTEXT.reset(__context_token)
cdef api void call_cy_MyServiceFast_lobDataById(
    object self,
    Cpp2RequestContext* ctx,
    cFollyPromise[cFollyUnit] cPromise,
    cint64_t id,
    string data
) noexcept:
    cdef Promise__sa_cFollyUnit __promise = Promise__sa_cFollyUnit._fbthrift_create(cmove(cPromise))
    arg_id = id
    arg_data = data.data().decode('UTF-8')
    __context = RequestContext._fbthrift_create(ctx)
    __context_token = __THRIFT_REQUEST_CONTEXT.set(__context)
    asyncio.get_event_loop().create_task(
        MyServiceFast_lobDataById_coro(
            self,
            __promise,
            arg_id,
            arg_data
        )
    )
    __THRIFT_REQUEST_CONTEXT.reset(__context_token)
cdef api void call_cy_MyServiceFast_onStartServing(
    object self,
    cFollyPromise[cFollyUnit] cPromise
) noexcept:
    cdef Promise__sa_cFollyUnit __promise = Promise__sa_cFollyUnit._fbthrift_create(cmove(cPromise))
    asyncio.get_event_loop().create_task(
        MyServiceFast_onStartServing_coro(
            self,
            __promise
        )
    )
cdef api void call_cy_MyServiceFast_onStopRequested(
    object self,
    cFollyPromise[cFollyUnit] cPromise
) noexcept:
    cdef Promise__sa_cFollyUnit __promise = Promise__sa_cFollyUnit._fbthrift_create(cmove(cPromise))
    asyncio.get_event_loop().create_task(
        MyServiceFast_onStopRequested_coro(
            self,
            __promise
        )
    )
async def MyServiceFast_hasDataById_coro(
    object self,
    Promise__sa_cbool promise,
    id
):
    try:
        result = await self.hasDataById(
                    id)
    except __ApplicationError as ex:
        # If the handler raised an ApplicationError convert it to a C++ one
        promise.cPromise.setException(cTApplicationException(
            ex.type.value, ex.message.encode('UTF-8')
        ))
    except Exception as ex:
        print(
            "Unexpected error in service handler MyServiceFast.hasDataById:",
            file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, repr(ex).encode('UTF-8')
        ))
    except asyncio.CancelledError as ex:
        print("Coroutine was cancelled in service handler MyServiceFast.hasDataById:", file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, (f'Application was cancelled on the server with message: {str(ex)}').encode('UTF-8')
        ))
    else:
        promise.cPromise.setValue(<cbool> result)

async def MyServiceFast_getDataById_coro(
    object self,
    Promise__sa_string promise,
    id
):
    try:
        result = await self.getDataById(
                    id)
    except __ApplicationError as ex:
        # If the handler raised an ApplicationError convert it to a C++ one
        promise.cPromise.setException(cTApplicationException(
            ex.type.value, ex.message.encode('UTF-8')
        ))
    except Exception as ex:
        print(
            "Unexpected error in service handler MyServiceFast.getDataById:",
            file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, repr(ex).encode('UTF-8')
        ))
    except asyncio.CancelledError as ex:
        print("Coroutine was cancelled in service handler MyServiceFast.getDataById:", file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, (f'Application was cancelled on the server with message: {str(ex)}').encode('UTF-8')
        ))
    else:
        promise.cPromise.setValue(<string?> result.encode('UTF-8'))

async def MyServiceFast_putDataById_coro(
    object self,
    Promise__sa_cFollyUnit promise,
    id,
    data
):
    try:
        result = await self.putDataById(
                    id,
                    data)
    except __ApplicationError as ex:
        # If the handler raised an ApplicationError convert it to a C++ one
        promise.cPromise.setException(cTApplicationException(
            ex.type.value, ex.message.encode('UTF-8')
        ))
    except Exception as ex:
        print(
            "Unexpected error in service handler MyServiceFast.putDataById:",
            file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, repr(ex).encode('UTF-8')
        ))
    except asyncio.CancelledError as ex:
        print("Coroutine was cancelled in service handler MyServiceFast.putDataById:", file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, (f'Application was cancelled on the server with message: {str(ex)}').encode('UTF-8')
        ))
    else:
        promise.cPromise.setValue(c_unit)

async def MyServiceFast_lobDataById_coro(
    object self,
    Promise__sa_cFollyUnit promise,
    id,
    data
):
    try:
        result = await self.lobDataById(
                    id,
                    data)
    except __ApplicationError as ex:
        # If the handler raised an ApplicationError convert it to a C++ one
        promise.cPromise.setException(cTApplicationException(
            ex.type.value, ex.message.encode('UTF-8')
        ))
    except Exception as ex:
        print(
            "Unexpected error in service handler MyServiceFast.lobDataById:",
            file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, repr(ex).encode('UTF-8')
        ))
    except asyncio.CancelledError as ex:
        print("Coroutine was cancelled in service handler MyServiceFast.lobDataById:", file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, (f'Application was cancelled on the server with message: {str(ex)}').encode('UTF-8')
        ))
    else:
        promise.cPromise.setValue(c_unit)

async def MyServiceFast_onStartServing_coro(
    object self,
    Promise__sa_cFollyUnit promise
):
    try:
        result = await self.onStartServing()
    except __ApplicationError as ex:
        # If the handler raised an ApplicationError convert it to a C++ one
        promise.cPromise.setException(cTApplicationException(
            ex.type.value, ex.message.encode('UTF-8')
        ))
    except Exception as ex:
        print(
            "Unexpected error in service handler MyServiceFast.onStartServing:",
            file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, repr(ex).encode('UTF-8')
        ))
    except asyncio.CancelledError as ex:
        print("Coroutine was cancelled in service handler MyServiceFast.onStartServing:", file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, (f'Application was cancelled on the server with message: {str(ex)}').encode('UTF-8')
        ))
    else:
        promise.cPromise.setValue(c_unit)

async def MyServiceFast_onStopRequested_coro(
    object self,
    Promise__sa_cFollyUnit promise
):
    try:
        result = await self.onStopRequested()
    except __ApplicationError as ex:
        # If the handler raised an ApplicationError convert it to a C++ one
        promise.cPromise.setException(cTApplicationException(
            ex.type.value, ex.message.encode('UTF-8')
        ))
    except Exception as ex:
        print(
            "Unexpected error in service handler MyServiceFast.onStopRequested:",
            file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, repr(ex).encode('UTF-8')
        ))
    except asyncio.CancelledError as ex:
        print("Coroutine was cancelled in service handler MyServiceFast.onStopRequested:", file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, (f'Application was cancelled on the server with message: {str(ex)}').encode('UTF-8')
        ))
    else:
        promise.cPromise.setValue(c_unit)

cdef api void call_cy_DbMixedStackArguments_getDataByKey0(
    object self,
    Cpp2RequestContext* ctx,
    cFollyPromise[unique_ptr[string]] cPromise,
    unique_ptr[string] key
) noexcept:
    cdef Promise_binary __promise = Promise_binary._fbthrift_create(cmove(cPromise))
    arg_key = (deref(key)).data().decode('UTF-8')
    __context = RequestContext._fbthrift_create(ctx)
    __context_token = __THRIFT_REQUEST_CONTEXT.set(__context)
    asyncio.get_event_loop().create_task(
        DbMixedStackArguments_getDataByKey0_coro(
            self,
            __promise,
            arg_key
        )
    )
    __THRIFT_REQUEST_CONTEXT.reset(__context_token)
cdef api void call_cy_DbMixedStackArguments_getDataByKey1(
    object self,
    Cpp2RequestContext* ctx,
    cFollyPromise[string] cPromise,
    string key
) noexcept:
    cdef Promise__sa_binary __promise = Promise__sa_binary._fbthrift_create(cmove(cPromise))
    arg_key = key.data().decode('UTF-8')
    __context = RequestContext._fbthrift_create(ctx)
    __context_token = __THRIFT_REQUEST_CONTEXT.set(__context)
    asyncio.get_event_loop().create_task(
        DbMixedStackArguments_getDataByKey1_coro(
            self,
            __promise,
            arg_key
        )
    )
    __THRIFT_REQUEST_CONTEXT.reset(__context_token)
cdef api void call_cy_DbMixedStackArguments_onStartServing(
    object self,
    cFollyPromise[cFollyUnit] cPromise
) noexcept:
    cdef Promise__sa_cFollyUnit __promise = Promise__sa_cFollyUnit._fbthrift_create(cmove(cPromise))
    asyncio.get_event_loop().create_task(
        DbMixedStackArguments_onStartServing_coro(
            self,
            __promise
        )
    )
cdef api void call_cy_DbMixedStackArguments_onStopRequested(
    object self,
    cFollyPromise[cFollyUnit] cPromise
) noexcept:
    cdef Promise__sa_cFollyUnit __promise = Promise__sa_cFollyUnit._fbthrift_create(cmove(cPromise))
    asyncio.get_event_loop().create_task(
        DbMixedStackArguments_onStopRequested_coro(
            self,
            __promise
        )
    )
async def DbMixedStackArguments_getDataByKey0_coro(
    object self,
    Promise_binary promise,
    key
):
    try:
        result = await self.getDataByKey0(
                    key)
    except __ApplicationError as ex:
        # If the handler raised an ApplicationError convert it to a C++ one
        promise.cPromise.setException(cTApplicationException(
            ex.type.value, ex.message.encode('UTF-8')
        ))
    except Exception as ex:
        print(
            "Unexpected error in service handler DbMixedStackArguments.getDataByKey0:",
            file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, repr(ex).encode('UTF-8')
        ))
    except asyncio.CancelledError as ex:
        print("Coroutine was cancelled in service handler DbMixedStackArguments.getDataByKey0:", file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, (f'Application was cancelled on the server with message: {str(ex)}').encode('UTF-8')
        ))
    else:
        promise.cPromise.setValue(make_unique[string](<string?> result))

async def DbMixedStackArguments_getDataByKey1_coro(
    object self,
    Promise__sa_binary promise,
    key
):
    try:
        result = await self.getDataByKey1(
                    key)
    except __ApplicationError as ex:
        # If the handler raised an ApplicationError convert it to a C++ one
        promise.cPromise.setException(cTApplicationException(
            ex.type.value, ex.message.encode('UTF-8')
        ))
    except Exception as ex:
        print(
            "Unexpected error in service handler DbMixedStackArguments.getDataByKey1:",
            file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, repr(ex).encode('UTF-8')
        ))
    except asyncio.CancelledError as ex:
        print("Coroutine was cancelled in service handler DbMixedStackArguments.getDataByKey1:", file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, (f'Application was cancelled on the server with message: {str(ex)}').encode('UTF-8')
        ))
    else:
        promise.cPromise.setValue(<string?> result)

async def DbMixedStackArguments_onStartServing_coro(
    object self,
    Promise__sa_cFollyUnit promise
):
    try:
        result = await self.onStartServing()
    except __ApplicationError as ex:
        # If the handler raised an ApplicationError convert it to a C++ one
        promise.cPromise.setException(cTApplicationException(
            ex.type.value, ex.message.encode('UTF-8')
        ))
    except Exception as ex:
        print(
            "Unexpected error in service handler DbMixedStackArguments.onStartServing:",
            file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, repr(ex).encode('UTF-8')
        ))
    except asyncio.CancelledError as ex:
        print("Coroutine was cancelled in service handler DbMixedStackArguments.onStartServing:", file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, (f'Application was cancelled on the server with message: {str(ex)}').encode('UTF-8')
        ))
    else:
        promise.cPromise.setValue(c_unit)

async def DbMixedStackArguments_onStopRequested_coro(
    object self,
    Promise__sa_cFollyUnit promise
):
    try:
        result = await self.onStopRequested()
    except __ApplicationError as ex:
        # If the handler raised an ApplicationError convert it to a C++ one
        promise.cPromise.setException(cTApplicationException(
            ex.type.value, ex.message.encode('UTF-8')
        ))
    except Exception as ex:
        print(
            "Unexpected error in service handler DbMixedStackArguments.onStopRequested:",
            file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, repr(ex).encode('UTF-8')
        ))
    except asyncio.CancelledError as ex:
        print("Coroutine was cancelled in service handler DbMixedStackArguments.onStopRequested:", file=sys.stderr)
        traceback.print_exc()
        promise.cPromise.setException(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, (f'Application was cancelled on the server with message: {str(ex)}').encode('UTF-8')
        ))
    else:
        promise.cPromise.setValue(c_unit)

