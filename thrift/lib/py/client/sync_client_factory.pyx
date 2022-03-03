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

# cython: c_string_type=unicode, c_string_encoding=utf8

import ipaddress
import os
import socket

from libc.stdint cimport uint32_t
from libcpp.utility cimport move as cmove
from libcpp.string cimport string
from thrift.py3lite.client.request_channel cimport (
    sync_createThriftChannelTCP,
    sync_createThriftChannelUnix,
    RequestChannel,
)

from thrift.py.client.common cimport (
    ClientType as cClientType,
    Protocol as cProtocol
)
from thrift.py.client.common import ClientType
from thrift.py.client.sync_client import SyncClient

def get_client(
    clientKlass,
    *,
    host=None,
    port=None,
    path=None,
    double timeout=1,
    cClientType client_type = ClientType.THRIFT_HEADER_CLIENT_TYPE,
    cProtocol protocol = cProtocol.COMPACT,
):
    if not issubclass(clientKlass, SyncClient):
        raise TypeError(f"{clientKlass} is not a py client class")

    endpoint = b""
    cdef uint32_t _timeout_ms = int(timeout * 1000)

    if host is not None and port is not None:
        if path is not None:
            raise ValueError("Can not set path and host/port at same time")

        if isinstance(host, str):
            try:
                ipaddress.ip_address(host)
            except ValueError:
                host = socket.getaddrinfo(host, port, type=socket.SOCK_STREAM)[0][4][0]
        else:
            host = str(host)

        channel = RequestChannel.create(sync_createThriftChannelTCP(
            host, port, _timeout_ms, client_type, protocol, endpoint
        ))
    elif path is not None:
        fspath = os.fsencode(path)
        channel = RequestChannel.create(sync_createThriftChannelUnix(
            cmove[string](fspath), _timeout_ms, client_type, protocol
        ))
    else:
        raise ValueError("Must set path or host/port")

    return clientKlass(channel)
