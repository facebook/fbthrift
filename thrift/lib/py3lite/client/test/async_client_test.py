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

from later.unittest import TestCase
from thrift.py3lite.async_client import AsyncClient, ClientType, get_client
from thrift.py3lite.serializer import Protocol

# @manual=//thrift/lib/py3lite/client/test:test_service-py3lite-types
from thrift.py3lite.test.lite_types import (
    _fbthrift_TestService_add_args,
    _fbthrift_TestService_add_result,
)
from thrift.py3lite.test.test_server import server_in_event_loop


class Client(AsyncClient):
    def __init__(self):
        super().__init__("TestService")

    async def add(self, num1: int, num2: int) -> int:
        resp = await self._send_request(
            "add",
            _fbthrift_TestService_add_args(num1=num1, num2=num2),
            _fbthrift_TestService_add_result,
        )
        return resp.success


class SyncClientTests(TestCase):
    async def test_basic(self) -> None:
        async with server_in_event_loop() as addr:
            async with get_client(Client, host=addr.ip, port=addr.port) as client:
                sum = await client.add(1, 2)
                self.assertEqual(3, sum)

    async def test_client_type_and_protocol(self) -> None:
        async with server_in_event_loop() as addr:
            async with get_client(
                Client,
                host=addr.ip,
                port=addr.port,
                client_type=ClientType.THRIFT_ROCKET_CLIENT_TYPE,
                protocol=Protocol.BINARY,
            ) as client:
                sum = await client.add(1, 2)
                self.assertEqual(3, sum)
