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

import ipaddress
import os
import socket

from libc.stdint cimport uint32_t
from thrift.py3lite.client cimport ssl as thrift_ssl
from thrift.py3lite.client.client_wrapper import Client
from thrift.py3lite.client.request_channel cimport (
    sync_createThriftChannelTCP,
    sync_createThriftChannelUnix,
    ClientType as cClientType,
    RequestChannel,
)
from thrift.py3lite.client.request_channel import ClientType
from thrift.py3lite.serializer cimport Protocol as cProtocol


def get_client(
    clientKlass,
    *,
    host=None,
    port=None,
    path=None,
    double timeout=1,
    cClientType client_type = ClientType.THRIFT_HEADER_CLIENT_TYPE,
    cProtocol protocol = cProtocol.COMPACT,
    thrift_ssl.SSLContext ssl_context=None,
    double ssl_timeout=1,
):
    if not issubclass(clientKlass, Client):
        raise TypeError(f"{clientKlass} is not a py3lite client class")

    endpoint = b''
    if client_type == ClientType.THRIFT_HTTP_CLIENT_TYPE:
        if host is None or port is None:
            raise ValueError("Must set host and port when using ClientType.THRIFT_HTTP_CLIENT_TYPE")
        if path is None:
            raise ValueError("use path='/endpoint' when using ClientType.THRIFT_HTTP_CLIENT_TYPE")
        endpoint = os.fsencode(path)
        path = None

    cdef uint32_t _timeout_ms = int(timeout * 1000)
    cdef uint32_t _ssl_timeout_ms = int(ssl_timeout * 1000)

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

        if ssl_context:
            channel = RequestChannel.create(thrift_ssl.sync_createThriftChannelTCP(
                ssl_context._cpp_obj,
                host,
                port,
                _timeout_ms,
                _ssl_timeout_ms,
                client_type,
                protocol,
                endpoint,
            ))
        else:
            channel = RequestChannel.create(sync_createThriftChannelTCP(
                host, port, _timeout_ms, client_type, protocol, endpoint
            ))
    elif path is not None:
        channel = RequestChannel.create(sync_createThriftChannelUnix(
            path, _timeout_ms, client_type, protocol
        ))
    else:
        raise ValueError("Must set path or host/port")
    return clientKlass.Sync(channel)
