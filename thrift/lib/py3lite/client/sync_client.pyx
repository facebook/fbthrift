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

# cython: c_string_type=unicode, c_string_encoding=utf8

from libcpp.memory cimport make_unique
from libcpp.utility cimport move as cmove
from cython.operator cimport dereference as deref
from thrift.py3lite.serializer import Protocol, serialize, deserialize
from thrift.py3lite.types import Struct, Union
cimport folly.iobuf

cdef class RequestChannel:
    @staticmethod
    cdef RequestChannel create(cRequestChannel_ptr channel):
        cdef RequestChannel inst = RequestChannel.__new__(RequestChannel)
        inst._cpp_obj = cmove(channel)
        return inst

cdef class SyncClient:
    def __init__(self, RequestChannel channel, string service_name):
        self._cpp_obj = make_unique[cOmniClient](
            cmove(channel._cpp_obj), service_name)

    def __enter__(self):
        return self

    def __exit__(self, exec_type, exc_value, traceback):
        # TODO: close channel
        pass

    def _send_request(
        self,
        string function_name,
        args,
        response_cls,
    ):
        protocol = Protocol(deref(self._cpp_obj).getChannelProtocolId())
        args_bytes = serialize(args, protocol=protocol)
        response_iobuf = folly.iobuf.from_unique_ptr(
            deref(self._cpp_obj).sync_send(function_name, args_bytes).buf
        )
        return deserialize(response_cls, response_iobuf, protocol=protocol)
