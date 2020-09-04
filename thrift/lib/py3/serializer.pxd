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

# distutils: language = c++

from libcpp.memory cimport unique_ptr
from libcpp.string cimport string
from libcpp.map cimport map
from libc.stdint cimport uint32_t, uint16_t
from folly.iobuf cimport cIOBuf, cIOBufQueue
from thrift.py3.common cimport Protocol as cProtocol


cdef extern from "thrift/lib/py3/serializer.h" namespace "::thrift::py3" nogil:
    cIOBufQueue cserialize "::thrift::py3::serialize"[T](const T* obj, cProtocol protocol) except +
    uint32_t cdeserialize"::thrift::py3::deserialize"[T](const cIOBuf* buf, T* obj, cProtocol protocol) except +


cdef extern from "thrift/lib/cpp/transport/THeader.h" namespace "apache::thrift::transport::THeader":
    cpdef enum Transform "apache::thrift::transport::THeader::TRANSFORMS":
        NONE,
        ZLIB_TRANSFORM,
        SNAPPY_TRANSFORM,
        ZSTD_TRANSFORM,

    cdef cppclass cTHeader "apache::thrift::transport::THeader":
        cTHeader() nogil except +
        void setProtocolId(cProtocol)
        uint16_t getProtocolId()
        void setTransform(Transform)
        unique_ptr[cIOBuf] addHeader(unique_ptr[cIOBuf], map[string, string])
        unique_ptr[cIOBuf] removeHeader(cIOBufQueue*, size_t&, map[string, string])
