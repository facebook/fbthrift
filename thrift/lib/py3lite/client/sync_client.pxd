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

from folly.iobuf cimport cIOBuf
from libc.stdint cimport uint16_t
from libcpp.memory cimport unique_ptr
from libcpp.string cimport string

cdef extern from "thrift/lib/py3lite/client/OmniClient.h" namespace "::thrift::py3lite::client":
    cdef cppclass cRequestChannel_ptr "::thrift::py3lite::client::RequestChannel_ptr":
        pass

    cdef cppclass cOmniClientResponseWithHeaders "::thrift::py3lite::client::OmniClientResponseWithHeaders":
        unique_ptr[cIOBuf] buf

    cdef cppclass cOmniClient "::thrift::py3lite::client::OmniClient":
        cOmniClient(cRequestChannel_ptr channel, const string& serviceName)
        cOmniClientResponseWithHeaders sync_send(const string& methodName, const string& args)
        uint16_t getChannelProtocolId()

cdef class RequestChannel:
    cdef cRequestChannel_ptr _cpp_obj
    @staticmethod
    cdef RequestChannel create(cRequestChannel_ptr channel)

cdef class SyncClient:
    cdef unique_ptr[cOmniClient] _cpp_obj
