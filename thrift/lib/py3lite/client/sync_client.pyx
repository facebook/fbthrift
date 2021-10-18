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

import time

from libc.stdint cimport uint32_t
from libcpp.memory cimport make_unique
from libcpp.utility cimport move as cmove
from cython.operator cimport dereference as deref
from thrift.py3lite.serializer import serialize, deserialize
from thrift.py3lite.serializer cimport Protocol as cProtocol
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
        protocol = deref(self._cpp_obj).getChannelProtocolId()
        args_bytes = serialize(args, protocol=protocol)
        if response_cls is None:
            # oneway
            deref(self._cpp_obj).oneway_send(function_name, args_bytes)
        else:
            response_iobuf = folly.iobuf.from_unique_ptr(
                deref(self._cpp_obj).sync_send(function_name, args_bytes).buf
            )
            return deserialize(response_cls, response_iobuf, protocol=protocol)


def get_client(
    clientKlass,
    *,
    host=None,
    port=None,
    path=None,
    double timeout=1,
    ClientType client_type = ClientType.THRIFT_HEADER_CLIENT_TYPE,
    cProtocol protocol = cProtocol.COMPACT,
):
    if not issubclass(clientKlass, SyncClient):
        if hasattr(clientKlass, "Sync"):
            clientKlass = clientKlass.Sync
    assert issubclass(clientKlass, SyncClient), "Must be a py3lite sync client"

    cdef uint32_t _timeout_ms = int(timeout * 1000)

    if host is not None and port is not None:
        if path is not None:
            raise ValueError("Can not set path and host/port at same time")
        channel = RequestChannel.create(createThriftChannelTCP(
            host, port, _timeout_ms, client_type, protocol
        ))
    elif path is not None:
        channel = RequestChannel.create(createThriftChannelUnix(
            path, _timeout_ms, client_type, protocol
        ))
    else:
        raise ValueError("Must set path or host/port")
    return clientKlass(channel)
