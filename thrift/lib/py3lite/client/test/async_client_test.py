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

import asyncio

import thrift.py3lite.types  # noqa: F401, TODO: remove after D31914637 lands
from later.unittest import TestCase
from thrift.py3lite.async_client import ClientType, get_client
from thrift.py3lite.exceptions import ApplicationError
from thrift.py3lite.serializer import Protocol
from thrift.py3lite.test.lite_clients import TestService
from thrift.py3lite.test.lite_types import ArithmeticException, EmptyException
from thrift.py3lite.test.test_server import server_in_event_loop


class SyncClientTests(TestCase):
    async def test_basic(self) -> None:
        async with server_in_event_loop() as addr:
            async with get_client(TestService, host=addr.ip, port=addr.port) as client:
                sum = await client.add(1, 2)
                self.assertEqual(3, sum)

    async def test_client_type_and_protocol(self) -> None:
        async with server_in_event_loop() as addr:
            async with get_client(
                TestService,
                host=addr.ip,
                port=addr.port,
                client_type=ClientType.THRIFT_ROCKET_CLIENT_TYPE,
                protocol=Protocol.BINARY,
            ) as client:
                sum = await client.add(1, 2)
                self.assertEqual(3, sum)

    async def test_void_return(self) -> None:
        async with server_in_event_loop() as addr:
            async with get_client(TestService, host=addr.ip, port=addr.port) as client:
                res = await client.noop()
                self.assertIsNone(res)

    async def test_exception(self) -> None:
        async with server_in_event_loop() as addr:
            async with get_client(TestService, host=addr.ip, port=addr.port) as client:
                res = await client.divide(6, 3)
                self.assertAlmostEqual(2, res)
                with self.assertRaises(ArithmeticException):
                    await client.divide(1, 0)

    async def test_void_return_with_exception(self) -> None:
        async with server_in_event_loop() as addr:
            async with get_client(TestService, host=addr.ip, port=addr.port) as client:
                with self.assertRaises(EmptyException):
                    await client.oops()

    async def test_oneway(self) -> None:
        async with server_in_event_loop() as addr:
            async with get_client(TestService, host=addr.ip, port=addr.port) as client:
                res = await client.oneway()
                self.assertIsNone(res)
                await asyncio.sleep(1)  # wait for server to clear the queue

    async def test_unexpected_exception(self) -> None:
        async with server_in_event_loop() as addr:
            async with get_client(TestService, host=addr.ip, port=addr.port) as client:
                with self.assertRaises(ApplicationError):
                    await client.surprise()
