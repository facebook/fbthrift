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

from later.unittest import TestCase
from thrift.py3lite.client import ClientType, get_client
from thrift.py3lite.exceptions import (
    ApplicationError,
    ApplicationErrorType,
    TransportError,
    TransportErrorType,
)
from thrift.py3lite.leaf.lite_clients import LeafService
from thrift.py3lite.serializer import Protocol
from thrift.py3lite.test.lite_clients import EchoService, TestService
from thrift.py3lite.test.lite_types import ArithmeticException, EmptyException
from thrift.py3lite.test.test_server import server_in_event_loop


TEST_HEADER_KEY = "headerKey"
TEST_HEADER_VALUE = "headerValue"


class AsyncClientTests(TestCase):
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
                with self.assertRaises(ApplicationError) as ex:
                    await client.surprise()
                self.assertEqual(ex.exception.message, "ValueError('Surprise!')")
                self.assertEqual(ex.exception.type, ApplicationErrorType.UNKNOWN)

    async def test_derived_service(self) -> None:
        async with server_in_event_loop() as addr:
            async with get_client(EchoService, host=addr.ip, port=addr.port) as client:
                out = await client.echo("hello")
                self.assertEqual("hello", out)
                sum = await client.add(1, 2)
                self.assertEqual(3, sum)

    async def test_deriving_from_external_service(self) -> None:
        async with server_in_event_loop() as addr:
            async with get_client(LeafService, host=addr.ip, port=addr.port) as client:
                rev = await client.reverse([1, 2, 3])
                self.assertEqual([3, 2, 1], list(rev))
                out = await client.echo("hello")
                self.assertEqual("hello", out)
                sum = await client.add(1, 2)
                self.assertEqual(3, sum)

    async def test_transport_error(self) -> None:
        async with get_client(TestService, path="/no/where") as client:
            with self.assertRaises(TransportError) as ex:
                await client.add(1, 2)
            self.assertEqual(TransportErrorType.UNKNOWN, ex.exception.type)

    async def test_hostname(self) -> None:
        async with server_in_event_loop() as addr:
            async with get_client(
                TestService, host="localhost", port=addr.port
            ) as client:
                sum = await client.add(1, 2)
                self.assertEqual(3, sum)

    async def test_persistent_header(self) -> None:
        async with server_in_event_loop() as addr:
            async with get_client(
                TestService, host="localhost", port=addr.port
            ) as client:
                client.set_persistent_header(TEST_HEADER_KEY, TEST_HEADER_VALUE)
                value = await client.readHeader(TEST_HEADER_KEY)
                self.assertEqual(TEST_HEADER_VALUE, value)
