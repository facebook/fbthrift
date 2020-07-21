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
import socket
import sys
import tempfile
import unittest
from pathlib import Path
from typing import Optional, Sequence

import thrift.py3.server
from derived.clients import DerivedTestingService
from derived.services import DerivedTestingServiceInterface
from folly.iobuf import IOBuf
from stack_args.clients import StackService
from stack_args.services import StackServiceInterface
from stack_args.types import simple
from testing.clients import TestingService
from testing.services import TestingServiceInterface
from testing.types import Color, HardError, easy
from thrift.py3 import (
    Protocol,
    RequestContext,
    RpcOptions,
    ThriftServer,
    TransportError,
    get_client,
    get_context,
)
from thrift.py3.client import ClientType
from thrift.py3.test.cpp_handler import CppHandler


class Handler(TestingServiceInterface):
    @TestingServiceInterface.pass_context_invert
    async def invert(self, ctx: RequestContext, value: bool) -> bool:
        if "from client" in ctx.read_headers:
            ctx.set_header("from server", "with love")
        return not value

    async def getName(self) -> str:
        if sys.version_info[:2] >= (3, 7):
            ctx = get_context()
            ctx.set_header("contextvar", "true")
        return "Testing"

    async def shutdown(self) -> None:
        pass

    async def complex_action(
        self, first: str, second: str, third: int, fourth: str
    ) -> int:
        return third

    async def takes_a_list(self, ints: Sequence[int]) -> None:
        pass

    async def take_it_easy(self, how: int, what: easy) -> None:
        pass

    async def pick_a_color(self, color: Color) -> None:
        pass

    async def int_sizes(self, one: int, two: int, three: int, four: int) -> None:
        pass

    async def hard_error(self, valid: bool) -> None:
        pass


class DerivedHandler(Handler, DerivedTestingServiceInterface):
    async def getName(self) -> str:
        return "DerivedTesting"

    async def derived_pick_a_color(self, color: Color) -> Color:
        return color


# pyre-fixme[13]: Attribute `serve_task` is never initialized.
class TestServer:
    server: ThriftServer
    serve_task: asyncio.Task

    def __init__(
        self,
        ip: Optional[str] = None,
        path: Optional["thrift.py3.server.Path"] = None,
        handler: thrift.py3.server.ServiceInterface = Handler(),  # noqa: B008
    ) -> None:
        self.server = ThriftServer(handler, ip=ip, path=path)

    async def __aenter__(self) -> thrift.py3.server.SocketAddress:
        self.serve_task = asyncio.get_event_loop().create_task(self.server.serve())
        return await self.server.get_address()

    # pyre-fixme[2]: Parameter must be annotated.
    async def __aexit__(self, *exc_info) -> None:
        self.server.stop()
        await self.serve_task


class ClientServerTests(unittest.TestCase):
    """
    These are tests where a client and server talk to each other
    """

    # pyre-fixme[56]: Argument `sys.version_info[slice(None, 2, None)] < (3, 7)` to
    #  decorator factory `unittest.skipIf` could not be resolved in a global scope.
    @unittest.skipIf(sys.version_info[:2] < (3, 7), "Requires py3.7")
    def test_get_context(self) -> None:
        loop = asyncio.get_event_loop()

        async def inner_test() -> None:
            async with TestServer(ip="::1") as sa:
                ip, port = sa.ip, sa.port
                assert ip and port
                async with get_client(TestingService, host=ip, port=port) as client:
                    options = RpcOptions()
                    self.assertEqual(
                        "Testing", await client.getName(rpc_options=options)
                    )
                    self.assertEqual("true", options.read_headers["contextvar"])

        loop.run_until_complete(inner_test())

        async def outside_context_test() -> None:
            handler = Handler()  # so we can call it outside the thrift server
            with self.assertRaises(LookupError):
                await handler.getName()

        loop.run_until_complete(outside_context_test())

    def test_rpc_headers(self) -> None:
        loop = asyncio.get_event_loop()

        async def inner_test() -> None:
            async with TestServer(ip="::1") as sa:
                ip, port = sa.ip, sa.port
                assert ip and port
                async with get_client(TestingService, host=ip, port=port) as client:
                    options = RpcOptions()
                    options.set_header("from client", "with love")
                    self.assertFalse(await client.invert(True, rpc_options=options))
                    self.assertIn("from server", options.read_headers)

        loop.run_until_complete(inner_test())

    def test_client_resolve(self) -> None:
        loop = asyncio.get_event_loop()
        hostname = socket.gethostname()

        # pyre-fixme[53]: Captured variable `hostname` is not annotated.
        async def inner_test() -> None:
            async with TestServer() as sa:
                port = sa.port
                assert port
                async with get_client(
                    TestingService, host=hostname, port=port
                ) as client:
                    self.assertTrue(await client.invert(False))
                    self.assertFalse(await client.invert(True))

        loop.run_until_complete(inner_test())

    def test_unframed_binary(self) -> None:
        loop = asyncio.get_event_loop()

        async def inner_test() -> None:
            async with TestServer(ip="::1") as sa:
                ip, port = sa.ip, sa.port
                assert ip and port
                async with get_client(
                    TestingService,
                    host=ip,
                    port=port,
                    client_type=ClientType.THRIFT_UNFRAMED_DEPRECATED,
                    protocol=Protocol.BINARY,
                ) as client:
                    self.assertTrue(await client.invert(False))
                    self.assertFalse(await client.invert(True))

        loop.run_until_complete(inner_test())

    def test_framed_deprecated(self) -> None:
        loop = asyncio.get_event_loop()

        async def inner_test() -> None:
            async with TestServer(ip="::1") as sa:
                ip, port = sa.ip, sa.port
                assert ip and port
                async with get_client(
                    TestingService,
                    host=ip,
                    port=port,
                    client_type=ClientType.THRIFT_FRAMED_DEPRECATED,
                ) as client:
                    self.assertTrue(await client.invert(False))
                    self.assertFalse(await client.invert(True))

        loop.run_until_complete(inner_test())

    def test_framed_compact(self) -> None:
        loop = asyncio.get_event_loop()

        async def inner_test() -> None:
            async with TestServer(ip="::1") as sa:
                ip, port = sa.ip, sa.port
                assert ip and port
                async with get_client(
                    TestingService,
                    host=ip,
                    port=port,
                    client_type=ClientType.THRIFT_FRAMED_COMPACT,
                ) as client:
                    self.assertTrue(await client.invert(False))
                    self.assertFalse(await client.invert(True))

        loop.run_until_complete(inner_test())

    def test_http_endpoint(self) -> None:
        loop = asyncio.get_event_loop()

        async def inner_test() -> None:
            async with TestServer(ip="::1") as sa:
                ip, port = sa.ip, sa.port
                assert ip and port
                async with get_client(
                    TestingService,
                    host=ip,
                    port=port,
                    path="/some/endpoint",
                    client_type=ClientType.THRIFT_HTTP_CLIENT_TYPE,
                ) as client:
                    try:
                        self.assertTrue(await client.invert(False))
                    except TransportError as err:
                        # The test server gets an invalid request because its a HTTP request
                        self.assertEqual(err.type.value, 4)  # END OF FILE

        loop.run_until_complete(inner_test())

    def test_server_localhost(self) -> None:
        loop = asyncio.get_event_loop()

        async def inner_test() -> None:
            async with TestServer(ip="::1") as sa:
                ip, port = sa.ip, sa.port
                assert ip and port
                async with get_client(TestingService, host=ip, port=port) as client:
                    self.assertTrue(await client.invert(False))
                    self.assertFalse(await client.invert(True))

        loop.run_until_complete(inner_test())

    def test_unix_socket(self) -> None:
        loop = asyncio.get_event_loop()

        async def inner_test(dir: Path) -> None:
            async with TestServer(path=dir / "tserver.sock") as sa:
                assert sa.path
                async with get_client(TestingService, path=sa.path) as client:
                    self.assertTrue(await client.invert(False))
                    self.assertFalse(await client.invert(True))

        with tempfile.TemporaryDirectory() as tdir:
            loop.run_until_complete(inner_test(Path(tdir)))

    def test_no_client_aexit(self) -> None:
        loop = asyncio.get_event_loop()

        async def inner_test() -> None:
            async with TestServer() as sa:
                ip, port = sa.ip, sa.port
                assert ip and port
                client = get_client(TestingService, host=ip, port=port)
                await client.__aenter__()
                self.assertTrue(await client.invert(False))
                self.assertFalse(await client.invert(True))

        # If we do not abort here then good

        loop.run_until_complete(inner_test())

    def test_client_aexit_no_await(self) -> None:
        """
        This actually handles the case if __aexit__ is not awaited
        """
        loop = asyncio.get_event_loop()

        async def inner_test() -> None:
            async with TestServer() as sa:
                ip, port = sa.ip, sa.port
                assert ip and port
                client = get_client(TestingService, host=ip, port=port)
                await client.__aenter__()
                self.assertTrue(await client.invert(False))
                self.assertFalse(await client.invert(True))
                fut = client.__aexit__(None, None, None)
                del client  # If we do not abort here then good

        loop.run_until_complete(inner_test())

    def test_no_client_no_aenter(self) -> None:
        """
        This covers if aenter was canceled since those two are the same really
        """
        loop = asyncio.get_event_loop()

        async def inner_test() -> None:
            async with TestServer() as sa:
                ip, port = sa.ip, sa.port
                assert ip and port
                get_client(TestingService, host=ip, port=port)

        # If we do not abort here then good

        loop.run_until_complete(inner_test())

    def test_derived_service(self) -> None:
        """
        This tests calling methods from a derived service
        """
        loop = asyncio.get_event_loop()

        async def inner_test() -> None:
            async with TestServer(handler=DerivedHandler()) as sa:
                ip, port = sa.ip, sa.port
                assert ip and port
                async with get_client(
                    DerivedTestingService, host=ip, port=port
                ) as client:
                    self.assertEqual(await client.getName(), "DerivedTesting")
                    self.assertEqual(
                        await client.derived_pick_a_color(Color.red), Color.red
                    )

        loop.run_until_complete(inner_test())

    def test_non_utf8_exception_message(self) -> None:
        loop = asyncio.get_event_loop()

        async def inner_test() -> None:
            async with TestServer(handler=CppHandler()) as sa:
                ip, port = sa.ip, sa.port
                assert ip and port
                async with get_client(TestingService, host=ip, port=port) as client:
                    with self.assertRaises(HardError):
                        await client.hard_error(True)
                    with self.assertRaises(UnicodeDecodeError):
                        await client.hard_error(False)

        loop.run_until_complete(inner_test())


class StackHandler(StackServiceInterface):
    async def add_to(self, lst: Sequence[int], value: int) -> Sequence[int]:
        return [x + value for x in lst]

    async def get_simple(self) -> simple:
        return simple(val=66)

    async def take_simple(self, smpl: simple) -> None:
        if smpl.val != 10:
            raise Exception("WRONG")

    async def get_iobuf(self) -> IOBuf:
        return IOBuf(b"abc")

    async def take_iobuf(self, val: IOBuf) -> None:
        if bytes(val) != b"cba":
            raise Exception("WRONG")

    # currently unsupported by cpp backend:
    # async def get_iobuf_ptr(self) -> IOBuf:
    #     return IOBuf(b'xyz')

    async def take_iobuf_ptr(self, val: IOBuf) -> None:
        if bytes(val) != b"zyx":
            raise Exception("WRONG")


class ClientStackServerTests(unittest.TestCase):
    """
    These are tests where a client and server(stack_arguments) talk to each other
    """

    def test_server_localhost(self) -> None:
        loop = asyncio.get_event_loop()

        async def inner_test() -> None:
            async with TestServer(handler=StackHandler(), ip="::1") as sa:
                ip, port = sa.ip, sa.port
                assert ip and port
                async with get_client(StackService, host=ip, port=port) as client:
                    self.assertEqual(
                        (3, 4, 5, 6), await client.add_to(lst=(1, 2, 3, 4), value=2)
                    )
                    self.assertEqual(66, (await client.get_simple()).val)
                    await client.take_simple(simple(val=10))
                    self.assertEqual(b"abc", bytes(await client.get_iobuf()))
                    await client.take_iobuf(IOBuf(b"cba"))
                    # currently unsupported by cpp backend:
                    # self.assertEqual(b'xyz', (await client.get_iobuf_ptr()))
                    await client.take_iobuf_ptr(IOBuf(b"zyx"))

        loop.run_until_complete(inner_test())
