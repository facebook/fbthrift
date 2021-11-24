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

import ipaddress
import typing

from thrift.py3lite.client.client_wrapper import Client, TAsyncClient, TSyncClient
from thrift.py3lite.client.request_channel import ClientType
from thrift.py3lite.client.ssl import SSLContext
from thrift.py3lite.client.sync_client import SyncClient
from thrift.py3lite.serializer import Protocol

def get_client(
    clientKlass: typing.Type[Client[TAsyncClient, TSyncClient]],
    *,
    host: typing.Optional[
        typing.Union[str, ipaddress.IPv4Address, ipaddress.IPv6Address]
    ] = ...,
    port: typing.Optional[int] = ...,
    path: typing.Optional[str] = ...,
    timeout: float = ...,
    client_type: ClientType = ...,
    protocol: Protocol = ...,
    ssl_context: typing.Optional[SSLContext] = ...,
    ssl_timeout: float = ...,
) -> TSyncClient: ...
