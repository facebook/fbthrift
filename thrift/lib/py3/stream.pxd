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


cdef extern from "thrift/lib/cpp2/async/SemiStream.h" namespace "::apache::thrift":
    cdef cppclass cSemiStream "::apache::thrift::SemiStream"[T]:
        pass

    cdef cppclass cResponseAndSemiStream "::apache::thrift::ResponseAndSemiStream"[R, S]:
        R response
        cSemiStream[S] stream

cdef extern from "thrift/lib/py3/stream.h" namespace "::thrift::py3":
    cdef cppclass cSemiStreamWrapper "::thrift::py3::SemiStreamWrapper"[T]:
        cSemiStreamWrapper() except +
        cSemiStreamWrapper(cSemiStream[T]&, int buffer_size) except +
        cFollyCoroTask[cOptional[T]] getNext()

    cdef cppclass cResponseAndSemiStreamWrapper "::thrift::py3::ResponseAndSemiStreamWrapper"[R, S]:
        cResponseAndSemiStreamWrapper() except +
        cResponseAndSemiStreamWrapper(cResponseAndSemiStream& rs)
        shared_ptr[R] getResponse() except +
        cSemiStream[S] getStream()

cdef class SemiStream:
    cdef cFollyExecutor* _executor

cdef class ResponseAndSemiStream:
    pass
