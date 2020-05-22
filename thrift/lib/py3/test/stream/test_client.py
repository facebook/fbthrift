#!/usr/bin/env python3
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
import unittest

from thrift.py3 import RpcOptions, get_client
from thrift.py3.client import ClientType
from thrift.py3.test.included.included.types import Included
from thrift.py3.test.server import TestServer
from thrift.py3.test.stream.clients import StreamTestService
from thrift.py3.test.stream.types import FuncEx, StreamEx


class StreamClientTest(unittest.TestCase):
    def test_return_stream(self) -> None:
        loop = asyncio.get_event_loop()

        async def inner_test() -> None:
            async with TestServer(ip="::1") as sa:
                ip, port = sa.ip, sa.port
                assert ip and port
                async with get_client(
                    StreamTestService,
                    host=ip,
                    port=port,
                    client_type=ClientType.THRIFT_ROCKET_CLIENT_TYPE,
                ) as client:
                    stream = await client.returnstream(10, 1024)
                    res = [n async for n in stream]
                    self.assertEqual(res, list(range(10, 1024)))

        loop.run_until_complete(inner_test())

    def test_stream_throws(self) -> None:
        loop = asyncio.get_event_loop()

        async def inner_test() -> None:
            async with TestServer(ip="::1") as sa:
                ip, port = sa.ip, sa.port
                assert ip and port
                async with get_client(
                    StreamTestService,
                    host=ip,
                    port=port,
                    client_type=ClientType.THRIFT_ROCKET_CLIENT_TYPE,
                ) as client:
                    with self.assertRaises(FuncEx):
                        await client.streamthrows(True)
                    stream = await client.streamthrows(False)
                    with self.assertRaises(StreamEx):
                        async for _ in stream:  # noqa: F841 current flake8 version too old to support "async for _ in"
                            pass

        loop.run_until_complete(inner_test())

    def test_return_response_and_stream(self) -> None:
        loop = asyncio.get_event_loop()

        async def inner_test() -> None:
            async with TestServer(ip="::1") as sa:
                ip, port = sa.ip, sa.port
                assert ip and port
                async with get_client(
                    StreamTestService,
                    host=ip,
                    port=port,
                    client_type=ClientType.THRIFT_ROCKET_CLIENT_TYPE,
                ) as client:
                    # pyre-fixme[23]: may be a pyre bug?
                    resp, stream = await client.returnresponseandstream(
                        Included(from_=39, to=42)
                    )
                    self.assertEqual(resp, Included(from_=100, to=200))
                    expected_to = 39
                    async for n in stream:
                        self.assertEqual(n, Included(from_=39, to=expected_to))
                        expected_to += 1
                    self.assertEqual(expected_to, 42)

        loop.run_until_complete(inner_test())

    def test_return_stream_set_buffer_size(self) -> None:
        loop = asyncio.get_event_loop()

        async def inner_test() -> None:
            options = RpcOptions()
            options.chunk_buffer_size = 64
            async with TestServer(ip="::1") as sa:
                ip, port = sa.ip, sa.port
                assert ip and port
                async with get_client(
                    StreamTestService,
                    host=ip,
                    port=port,
                    client_type=ClientType.THRIFT_ROCKET_CLIENT_TYPE,
                ) as client:
                    stream = await client.returnstream(10, 1024, rpc_options=options)
                    res = [n async for n in stream]
                    self.assertEqual(res, list(range(10, 1024)))

        loop.run_until_complete(inner_test())
