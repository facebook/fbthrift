# Copyright (c) Meta Platforms, Inc. and affiliates.
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

from cpython.ref cimport PyObject
from libcpp.memory cimport unique_ptr
from libcpp.string cimport string
from libcpp.map cimport map as cmap
from libcpp.pair cimport pair
from libcpp.vector cimport vector as cvector
from folly.iobuf cimport cIOBuf
from thrift.python.exceptions cimport cException
from thrift.python.protocol cimport Protocol, RpcKind
from thrift.python.server_impl.request_context cimport Cpp2RequestContext
from thrift.python.types cimport ServiceInterface as cServiceInterface
from thrift.python.server_impl.async_processor cimport (
    cAsyncProcessorFactory,
    AsyncProcessorFactory,
)
from thrift.python.std_libcpp cimport string_view
from libcpp.memory cimport shared_ptr
from libcpp cimport bool as cbool
from folly.executor cimport cAsyncioExecutor

# cython doesn't support * in template parameters
# Make a typedef to workaround this.
ctypedef PyObject* PyObjPtr
# Keep the exported Cython callback as a function pointer. C++ converts it to
# StartControlRequest at the ExecutionSystem constructor boundary.
ctypedef int (*cStartControlRequest)(PyObject*) except -1


cdef extern from "thrift/lib/python/server/execution/RequestExecution.h" namespace "::apache::thrift::python::execution":
    cdef cppclass cRequestExecution "::apache::thrift::python::execution::RequestExecution":
        int operator()(PyObject* coroutineFactory) except -1


cdef extern from "thrift/lib/cpp2/async/AsyncProcessorFactory.h" namespace "::apache::thrift::AsyncProcessorFactory::MethodMetadata":
    cdef enum cInteractionType "::apache::thrift::AsyncProcessorFactory::MethodMetadata::InteractionType":
        # Explicit values are required: the C++ enum is
        #   { UNKNOWN=0, NONE=1, INTERACTION_V1=2 } and Cython would otherwise
        # assign 0, 1 in declaration order.
        cInteractionType_NONE "::apache::thrift::AsyncProcessorFactory::MethodMetadata::InteractionType::NONE" = 1
        cInteractionType_INTERACTION_V1 "::apache::thrift::AsyncProcessorFactory::MethodMetadata::InteractionType::INTERACTION_V1" = 2

cdef extern from "thrift/lib/python/server/PythonAsyncProcessor.h" namespace "::apache::thrift::python":
    struct HandlerFunc:
        RpcKind kind
        PyObjPtr funcObject
        string fullName
        string_view interactionName
        cInteractionType interactionType
        cbool createsInteraction
        cbool returnsInitialResponse
        PyObjPtr factoryObject

    HandlerFunc makeHandlerFunc(
        RpcKind kind,
        PyObjPtr funcObject,
        const string& serviceName,
        string_view functionName,
    )

    HandlerFunc makeInteractionHandlerFunc(
        RpcKind kind,
        PyObjPtr funcObject,
        const string& serviceName,
        string_view functionName,
        string_view interactionName,
        cbool createsInteraction,
        PyObjPtr factoryObject,
        cbool returnsInitialResponse,
    )

cdef extern from "thrift/lib/python/server/execution/ExecutionSystem.h" namespace "::apache::thrift::python::execution":
    cdef cppclass cExecutionSystem "::apache::thrift::python::execution::ExecutionSystem":
        cExecutionSystem(
            cAsyncioExecutor* controlExecutor,
            cStartControlRequest startRequest,
        ) except +

cdef extern from "thrift/lib/python/server/PythonAsyncProcessorFactory.h" namespace "::apache::thrift::python":
    cdef cppclass cPythonAsyncProcessorFactory "::apache::thrift::python::PythonAsyncProcessorFactory"(cAsyncProcessorFactory):
        void releaseOwnedResources() noexcept
        void markContextEntered() noexcept

    cdef shared_ptr[cPythonAsyncProcessorFactory] \
        cCreatePythonAsyncProcessorFactory "::apache::thrift::python::PythonAsyncProcessorFactory::create"(
            PyObject* server,
            cmap[string_view, HandlerFunc] funcs,
            cvector[PyObjPtr] lifecycle,
            shared_ptr[cExecutionSystem] executionSystem,
            cAsyncioExecutor* controlExecutor,
            string serviceName,
        ) except +

cdef extern from "thrift/lib/cpp2/async/RpcTypes.h" namespace "::apache::thrift":
    cdef cppclass SerializedRequest "::apache::thrift::SerializedRequest":
        unique_ptr[cIOBuf] buffer

cdef extern from "thrift/lib/python/server/PythonAsyncProcessor.h" namespace "::apache::thrift::python":
    cdef cppclass RequestDispatchParameters:
        Protocol protocol
        Cpp2RequestContext* requestContext
        SerializedRequest serializedRequest
        RpcKind rpcKind
        const HandlerFunc* function
        PyObjPtr handlerFunction
        cRequestExecution requestExecution

cdef class PythonAsyncProcessorFactory(AsyncProcessorFactory):
    cdef dict funcMap
    cdef list lifecycleFuncs
    cdef object handler

    cdef void releaseOwnedResources(self) noexcept

    @staticmethod
    cdef PythonAsyncProcessorFactory create(cServiceInterface server)
