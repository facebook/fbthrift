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

cimport cython
from cpython.ref cimport PyObject
from libcpp cimport bool as cbool
from libcpp.map cimport map as cmap
from libcpp.memory cimport shared_ptr, unique_ptr
from libcpp.pair cimport pair
from libcpp.string cimport string
from libcpp.vector cimport vector as cvector

from folly cimport cFollyPromise, cFollyUnit, c_unit
from folly.executor cimport cAsyncioExecutor
from folly.iobuf cimport cIOBuf

from thrift.py3.stream cimport (
    cResponseAndServerStream,
    cServerStream,
    ServerStream,
)
from thrift.python.exceptions cimport (
    cException,
    cTApplicationException,
)
from thrift.python.protocol cimport RpcKind
from thrift.python.server_impl.async_processor cimport (
    cAsyncProcessorFactory,
    AsyncProcessorFactory,
)
from thrift.python.streaming.py_promise cimport Promise_Py
from thrift.python.streaming.python_user_exception cimport cPythonUserException
from thrift.python.streaming.sink cimport (
    cResponseAndSinkConsumer,
    cSinkConsumer,
)
from thrift.python.types cimport ServiceInterface as cServiceInterface


# cython doesn't support * in template parameters
# Make a typedef to workaround this.
ctypedef PyObject* PyObjPtr

ctypedef unique_ptr[cIOBuf] UniqueIOBuf
ctypedef cResponseAndSinkConsumer[UniqueIOBuf, UniqueIOBuf, UniqueIOBuf] SinkResponse
ctypedef cResponseAndServerStream[UniqueIOBuf, UniqueIOBuf] StreamResponse


cdef extern from "thrift/lib/python/server/PythonAsyncProcessorFactory.h" namespace "::apache::thrift::python":
    cdef cppclass cPythonAsyncProcessorFactory "::apache::thrift::python::PythonAsyncProcessorFactory"(cAsyncProcessorFactory):
        pass

    cdef shared_ptr[cPythonAsyncProcessorFactory] \
        cCreatePythonAsyncProcessorFactory "::apache::thrift::python::PythonAsyncProcessorFactory::create"(
            PyObject* server,
            cmap[string, pair[RpcKind, PyObjPtr]] funcs,
            cvector[PyObjPtr] lifecycle,
            cAsyncioExecutor* executor,
            string serviceName,
        ) except +

cdef extern from "thrift/lib/cpp2/async/RpcTypes.h" namespace "::apache::thrift":
    cdef cppclass SerializedRequest "::apache::thrift::SerializedRequest":
        unique_ptr[cIOBuf] buffer

cdef class PythonAsyncProcessorFactory(AsyncProcessorFactory):
    cdef dict funcMap
    cdef list lifecycleFuncs

    @staticmethod
    cdef PythonAsyncProcessorFactory create(cServiceInterface server)

cdef class Promise_cFollyUnit(Promise_Py):
    cdef cFollyPromise[cFollyUnit]* cPromise

    cdef error_ta(Promise_cFollyUnit self, cTApplicationException err)
    cdef error_py(Promise_cFollyUnit self, cPythonUserException err)
    cdef complete(Promise_cFollyUnit self, object _)

    @staticmethod
    cdef create(cFollyPromise[cFollyUnit] cPromise)

cdef class Promise_Sink(Promise_Py):
    cdef cFollyPromise[SinkResponse]* _cPromise

    cdef error_ta(Promise_Sink self, cTApplicationException err)
    cdef error_py(Promise_Sink self, cPythonUserException err)
    cdef complete(Promise_Sink self, object pyobj)
    
    @staticmethod
    cdef create(cFollyPromise[SinkResponse] promise)

cdef class Promise_Stream(Promise_Py):
    cdef cFollyPromise[StreamResponse]* cPromise

    cdef error_ta(Promise_Stream self, cTApplicationException err)
    cdef error_py(Promise_Stream self, cPythonUserException err)
    cdef complete(Promise_Stream self, object pyobj)
    
    @staticmethod
    cdef create(cFollyPromise[StreamResponse] cPromise)
    
cdef class ResponseAndServerStream:
    cdef unique_ptr[StreamResponse] cResponseStream

    @staticmethod
    cdef _fbthrift_create(object val, object stream)

cdef class ResponseAndSinkConsumer:
    cdef unique_ptr[SinkResponse] _cResponseSink
    
    @staticmethod
    cdef _fbthrift_create(object val, object sink)

@cython.final
cdef class ServerSink_IOBuf:
    cdef unique_ptr[cSinkConsumer[UniqueIOBuf, UniqueIOBuf]] _cSink
    
    @staticmethod
    cdef _fbthrift_create(object sink_callback)

cdef class ServerStream_IOBuf(ServerStream):
    cdef unique_ptr[cServerStream[UniqueIOBuf]] cStream

    @staticmethod
    cdef _fbthrift_create(object stream)
