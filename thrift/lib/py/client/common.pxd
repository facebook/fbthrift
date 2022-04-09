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

cdef extern from "thrift/lib/cpp/transport/THeader.h":
    cpdef enum ClientType "CLIENT_TYPE":
        THRIFT_HEADER_CLIENT_TYPE
        THRIFT_ROCKET_CLIENT_TYPE

cdef extern from "<thrift/lib/cpp/protocol/TProtocolTypes.h>" namespace "apache::thrift::protocol":
    cpdef enum Protocol "apache::thrift::protocol::PROTOCOL_TYPES":
        BINARY "apache::thrift::protocol::T_BINARY_PROTOCOL"
        COMPACT "apache::thrift::protocol::T_COMPACT_PROTOCOL"

from thrift.python.client.request_channel cimport RequestChannel
from thrift.python.client.request_channel cimport cRequestChannel_ptr
