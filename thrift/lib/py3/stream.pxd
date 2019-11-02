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

from libcpp.memory cimport unique_ptr, shared_ptr
from folly cimport cFollyExecutor
from folly.coro cimport cFollyCoroTask
from folly.optional cimport cOptional
from folly.async_generator cimport cAsyncGeneratorWrapper, cAsyncGenerator
from thrift.py3.common cimport RpcOptions

cdef extern from "thrift/lib/cpp2/async/ClientBufferedStream.h" namespace "::apache::thrift":
    cdef cppclass cClientBufferedStream "::apache::thrift::ClientBufferedStream"[T]:
         cClientBufferedStream()

    cdef cppclass cResponseAndClientBufferedStream "::apache::thrift::ResponseAndClientBufferedStream"[R, S]:
        R response
        cClientBufferedStream[S] stream
        cResponseAndClientBufferedStream()

cdef extern from "thrift/lib/cpp2/async/Stream.h" namespace "::apache::thrift":
    cdef cppclass cStream "::apache::thrift::Stream"[T]:
        cStream()

    cdef cppclass cResponseAndStream "::apache::thrift::ResponseAndStream"[R, S]:
        R response
        cStream[S] stream
        cResponseAndStream()

cdef extern from "thrift/lib/py3/stream.h" namespace "::thrift::py3":
    cdef cppclass cClientBufferedStreamWrapper "::thrift::py3::ClientBufferedStreamWrapper"[T]:
        cClientBufferedStreamWrapper() except +
        cClientBufferedStreamWrapper(cClientBufferedStream[T]&, int buffer_size) except +
        cFollyCoroTask[cOptional[T]] getNext()


cdef class ClientBufferedStream:
    cdef cFollyExecutor* _executor
    cdef RpcOptions _rpc_options

cdef class ResponseAndClientBufferedStream:
    pass

cdef class Stream:
    pass

cdef class ResponseAndStream:
    pass
