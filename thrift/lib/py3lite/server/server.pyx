# Copyright (c) Facebook, Inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import asyncio
import sys
import traceback

from cpython.ref cimport PyObject
from libcpp.map cimport map as cmap
from libcpp.memory cimport make_shared, static_pointer_cast
from libcpp.string cimport string
from libcpp.unordered_set cimport unordered_set
from libcpp.utility cimport move as cmove
from folly.executor cimport get_executor
from folly.iobuf cimport IOBuf, from_unique_ptr
from thrift.py3.exceptions cimport cTApplicationException, cTApplicationExceptionType__UNKNOWN, ApplicationError
from thrift.py3.server cimport Cpp2RequestContext, ThriftServer as ThriftServer_py3, RequestContext, THRIFT_REQUEST_CONTEXT
from thrift.py3lite.serializer cimport Protocol
from folly cimport (
  cFollyPromise,
  cFollyUnit,
  c_unit,
)

def oneway(func):
    func.is_one_way = True
    return func

cdef class Promise_Py:
    cdef error(Promise_Py self, cTApplicationException err):
        pass

    cdef complete(Promise_Py self, object pyobj):
        pass

cdef class Promise_IOBuf(Promise_Py):
    cdef cFollyPromise[unique_ptr[cIOBuf]]* cPromise

    def __cinit__(self):
        self.cPromise = new cFollyPromise[unique_ptr[cIOBuf]](cFollyPromise[unique_ptr[cIOBuf]].makeEmpty())

    def __dealloc__(self):
        del self.cPromise

    cdef error(Promise_IOBuf self, cTApplicationException err):
        self.cPromise.setException(err)

    cdef complete(Promise_IOBuf self, object pyobj):
        self.cPromise.setValue(cmove((<IOBuf>pyobj)._ours))

    @staticmethod
    cdef create(cFollyPromise[unique_ptr[cIOBuf]] cPromise):
        cdef Promise_IOBuf inst = Promise_IOBuf.__new__(Promise_IOBuf)
        inst.cPromise[0] = cmove(cPromise)
        return inst

cdef class Promise_cFollyUnit(Promise_Py):
    cdef cFollyPromise[cFollyUnit]* cPromise

    def __cinit__(self):
        self.cPromise = new cFollyPromise[cFollyUnit](cFollyPromise[cFollyUnit].makeEmpty())

    def __dealloc__(self):
        del self.cPromise

    cdef error(Promise_cFollyUnit self, cTApplicationException err):
        self.cPromise.setException(err)

    cdef complete(Promise_cFollyUnit self, object _):
        self.cPromise.setValue(c_unit)

    @staticmethod
    cdef create(cFollyPromise[cFollyUnit] cPromise):
        cdef Promise_cFollyUnit inst = Promise_cFollyUnit.__new__(Promise_cFollyUnit)
        inst.cPromise[0] = cmove(cPromise)
        return inst

async def serverCallback_coro(object callFunc, str funcName, Promise_Py promise, IOBuf buf, Protocol prot):
    try:
        val = await callFunc(buf, prot)
    except ApplicationError as ex:
        # If the handler raised an ApplicationError convert it to a C++ one
        promise.error(cTApplicationException(
            ex.type.value, ex.message.encode('UTF-8')
        ))
    except Exception as ex:
        print(
            f"Unexpected error in {funcName}:",
            file=sys.stderr)
        traceback.print_exc()
        promise.error(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, repr(ex).encode('UTF-8')
        ))
    except asyncio.CancelledError as ex:
        print(f"Coroutine was cancelled in service handler {funcName}:", file=sys.stderr)
        traceback.print_exc()
        promise.error(cTApplicationException(
            cTApplicationExceptionType__UNKNOWN, (f'Application was cancelled on the server with message: {str(ex)}').encode('UTF-8')
        ))
    else:
        promise.complete(val)

cdef void combinedHandler(object func, string funcName, Cpp2RequestContext* ctx, Promise_Py promise, SerializedRequest serializedRequest, Protocol prot):
    __context = RequestContext._fbthrift_create(ctx)
    __context_token = THRIFT_REQUEST_CONTEXT.set(__context)

    asyncio.get_event_loop().create_task(
        serverCallback_coro(
            func,
            funcName.decode('UTF-8'),
            promise,
            from_unique_ptr(cmove(serializedRequest.buffer)),
            prot,
        )
    )

    THRIFT_REQUEST_CONTEXT.reset(__context_token)

cdef public api void handleServerCallback(object func, string funcName, Cpp2RequestContext* ctx, cFollyPromise[unique_ptr[cIOBuf]] cPromise, SerializedRequest serializedRequest, Protocol prot):
    cdef Promise_IOBuf __promise = Promise_IOBuf.create(cmove(cPromise))
    combinedHandler(func, funcName, ctx, __promise, cmove(serializedRequest), prot)


cdef public api void handleServerCallbackOneway(object func, string funcName, Cpp2RequestContext* ctx, cFollyPromise[cFollyUnit] cPromise, SerializedRequest serializedRequest, Protocol prot):
    cdef Promise_cFollyUnit __promise = Promise_cFollyUnit.create(cmove(cPromise))
    combinedHandler(func, funcName, ctx, __promise, cmove(serializedRequest), prot)

cdef class Py3LiteAsyncProcessorFactory(AsyncProcessorFactory):
    @staticmethod
    cdef Py3LiteAsyncProcessorFactory create(dict funcMap, bytes serviceName):
        cdef cmap[string, PyObject*] funcs
        cdef unordered_set[string] oneways
        for name, func in funcMap.items():
            funcs[<string>name] = <PyObject*>func
            if getattr(func, "is_one_way", False):
                oneways.insert(name)
        cdef Py3LiteAsyncProcessorFactory inst = Py3LiteAsyncProcessorFactory.__new__(Py3LiteAsyncProcessorFactory)
        inst._cpp_obj = static_pointer_cast[cAsyncProcessorFactory, cPy3LiteAsyncProcessorFactory](
            make_shared[cPy3LiteAsyncProcessorFactory](cmove(funcs), cmove(oneways), get_executor(), serviceName))
        return inst

cdef class ServiceInterface:
    @staticmethod
    def service_name():
        raise NotImplementedError("Service name not implemented")

    def getFunctionTable(self):
        return {}

    async def __aenter__(self):
        # Establish async context managers as a way for end users to async initalize
        # internal structures used by Service Handlers.
        return self

    async def __aexit__(self, *exc_info):
        # Same as above, but allow end users to define things to be cleaned up
        pass

cdef class ThriftServer(ThriftServer_py3):
    cdef readonly dict funcMap
    cdef readonly ServiceInterface handler

    def __init__(self, ServiceInterface server, int port=0, ip=None, path=None):
        self.funcMap = server.getFunctionTable()
        self.handler = server
        super().__init__(Py3LiteAsyncProcessorFactory.create(self.funcMap, self.handler.service_name()), port, ip, path)
