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

# distutils: language=c++

from libcpp.map cimport map as cmap
from libc.stdint cimport int32_t, int64_t
from libcpp.string cimport string
from thrift.py3.std_libcpp cimport milliseconds


cdef extern from "thrift/lib/cpp/protocol/TProtocolTypes.h" namespace "apache::thrift::protocol":
    cdef enum PROTOCOL_TYPES:
        T_BINARY_PROTOCOL,
        T_JSON_PROTOCOL,
        T_COMPACT_PROTOCOL,
        T_DEBUG_PROTOCOL,
        T_VIRTUAL_PROTOCOL,
        T_SIMPLE_JSON_PROTOCOL,


cdef inline PROTOCOL_TYPES Protocol2PROTOCOL_TYPES(object proto):
    cdef int value = proto.value
    if value == 0:
        return PROTOCOL_TYPES.T_COMPACT_PROTOCOL
    elif value == 1:
        return PROTOCOL_TYPES.T_BINARY_PROTOCOL
    elif value == 3:
        return PROTOCOL_TYPES.T_SIMPLE_JSON_PROTOCOL
    elif value == 4:
        return PROTOCOL_TYPES.T_JSON_PROTOCOL


cdef extern from "thrift/lib/cpp/concurrency/Thread.h":

    enum cPriority "apache::thrift::concurrency::PRIORITY":
        cHIGH_IMPORTANT "apache::thrift::concurrency::HIGH_IMPORTANT"
        cHIGH "apache::thrift::concurrency::HIGH"
        cIMPORTANT "apache::thrift::concurrency::IMPORTANT"
        cNORMAL "apache::thrift::concurrency::NORMAL"
        cBEST_EFFORT "apache::thrift::concurrency::BEST_EFFORT"
        cN_PRIORITIES "apache::thrift::concurrency::N_PRIORITIES"


cdef inline cPriority Priority_to_cpp(object value):
    cdef int cvalue = value.value
    return <cPriority>cvalue


cdef class Headers:
    cdef object __weakref__
    cdef const cmap[string, string]* _getMap(self)


cdef extern from "thrift/lib/cpp2/async/RequestChannel.h" namespace "apache::thrift":
    cdef cppclass cRpcOptions "apache::thrift::RpcOptions":
        cRpcOptions()
        cRpcOptions& setTimeout(milliseconds timeout)
        milliseconds getTimeout()
        cRpcOptions& setPriority(cPriority priority)
        cPriority getPriority()
        cRpcOptions& setChunkTimeout(milliseconds timeout)
        milliseconds getChunkTimeout()
        cRpcOptions& setQueueTimeout(milliseconds timeout)
        milliseconds getQueueTimeout()
        cRpcOptions& setChunkBufferSize(int32_t chunkBufferSize)
        int32_t getChunkBufferSize()
        void setWriteHeader(const string& key, const string value)
        const cmap[string, string]& getReadHeaders()
        const cmap[string, string]& getWriteHeaders()


cdef class RpcOptions:
    cdef object __weakref__
    cdef object _readheaders
    cdef object _writeheaders
    cdef cRpcOptions _cpp_obj


cdef class ReadHeaders(Headers):
    cdef RpcOptions _parent
    @staticmethod
    cdef create(RpcOptions rpc_options)


cdef class WriteHeaders(Headers):
    cdef RpcOptions _parent
    @staticmethod
    cdef create(RpcOptions rpc_options)
