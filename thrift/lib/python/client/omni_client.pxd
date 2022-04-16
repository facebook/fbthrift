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

from folly cimport cFollyExceptionWrapper, cFollySemiFuture
from folly.expected cimport cExpected
from folly.iobuf cimport cIOBuf
from libc.stdint cimport uint16_t
from libcpp.map cimport map as cmap
from libcpp.memory cimport unique_ptr
from libcpp.string cimport string
from libcpp.unordered_map cimport unordered_map
from thrift.python.client.request_channel cimport cRequestChannel_ptr

cdef extern from "thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h" namespace "::apache::thrift":
    cpdef enum class RpcKind:
        SINGLE_REQUEST_SINGLE_RESPONSE = 0,
        SINGLE_REQUEST_NO_RESPONSE = 1,
        SINGLE_REQUEST_STREAMING_RESPONSE = 4,
        SINK = 6,

cdef extern from "thrift/lib/python/client/OmniClient.h" namespace "::thrift::python::client":
    cdef cppclass cOmniClientResponseWithHeaders "::thrift::python::client::OmniClientResponseWithHeaders":
        cExpected[unique_ptr[cIOBuf], cFollyExceptionWrapper] buf
        cmap[string, string] headers

    cdef cppclass cOmniClient "::thrift::python::client::OmniClient":
        cOmniClient(cRequestChannel_ptr channel, const string& serviceName)
        cOmniClientResponseWithHeaders sync_send(
            const string& serviceName,
            const string& methodName,
            unique_ptr[cIOBuf] args,
            const unordered_map[string, string] headers,
        )
        void oneway_send(
            const string& serviceName,
            const string& methodName,
            unique_ptr[cIOBuf] args,
            const unordered_map[string, string] headers,
        )
        cFollySemiFuture[cOmniClientResponseWithHeaders] semifuture_send(
            const string& serviceName,
            const string& methodName,
            unique_ptr[cIOBuf] args,
            const unordered_map[string, string] headers,
            const RpcKind rpcKind,
        )
        uint16_t getChannelProtocolId()
