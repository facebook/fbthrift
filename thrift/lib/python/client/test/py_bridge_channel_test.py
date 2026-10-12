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


from __future__ import annotations

import asyncio
from enum import Enum
from unittest import IsolatedAsyncioTestCase

from thrift.python.client.py_bridge.py_bridge_channel import get_bridged_async_client
from thrift.python.exceptions import (
    ApplicationError,
    ApplicationErrorType,
    TransportError,
    TransportErrorType,
    TransportOptions,
)
from thrift.python.serializer import Protocol, serialize
from thrift.python.test.thrift_clients import EchoService, TestService
from thrift.python.test.thrift_types import (
    _fbthrift_EchoService_echo_args,
    _fbthrift_EchoService_echo_result,
)


# Test-only Compact message-envelope helpers (production enveloping is C++).
def _write_varint32(buf: bytearray, value: int) -> None:
    # 32-bit varint: the mask is intentional (seqid / name length only).
    value &= 0xFFFFFFFF
    while True:
        byte = value & 0x7F
        value >>= 7
        buf.append(byte | 0x80 if value else byte)
        if not value:
            return


def _read_varint(data: bytes, pos: int) -> tuple[int, int]:
    result = 0
    shift = 0
    while True:
        byte = data[pos]
        pos += 1
        result |= (byte & 0x7F) << shift
        if not (byte & 0x80):
            return result, pos
        shift += 7


def _compact_reply(method: str, body: bytes) -> bytes:
    # 0x82 proto id; version (fbthrift VERSION_N = 2) | (REPLY type << 5).
    out = bytearray([0x82, 0x02 | (2 << 5)])
    _write_varint32(out, 0)  # seqid
    name = method.encode("utf-8")
    _write_varint32(out, len(name))
    out += name
    out += body
    return bytes(out)


def _compact_method(data: bytes) -> str:
    # Mirror of the Compact CALL envelope `LegacySerializedRequest` writes:
    # [0]=proto id, [1]=version|type, then varint seqid, varint name length,
    # name bytes. Start at byte 2 (after proto id + version|type byte).
    _seqid, pos = _read_varint(data, 2)
    name_len, pos = _read_varint(data, pos)
    return data[pos : pos + name_len].decode("utf-8")


def _compact_body(data: bytes) -> bytes:
    _seqid, pos = _read_varint(data, 2)
    name_len, pos = _read_varint(data, pos)
    return data[pos + name_len :]


class _CannedHandler:
    """A ChannelHandler that records requests and returns a fixed reply."""

    def __init__(self, reply: bytes = b"") -> None:
        self.reply = reply
        self.requests: list[tuple[bytes, int]] = []
        self.called: asyncio.Event = asyncio.Event()

    async def send_request(self, enveloped_request: bytes, rpc_kind: int) -> bytes:
        self.requests.append((enveloped_request, rpc_kind))
        self.called.set()
        return self.reply


class _RaisingHandler:
    def __init__(self, error: BaseException) -> None:
        self.error = error

    async def send_request(self, enveloped_request: bytes, rpc_kind: int) -> bytes:
        raise self.error


class _SubclassedTransportError(TransportError):
    """Stands in for a library subclass such as ServiceRouterError."""


class _NotATransportErrorType(Enum):
    BOGUS = 999


_LOGGER = "thrift.python.client.py_bridge.py_bridge_channel"


class PyBridgeChannelTest(IsolatedAsyncioTestCase):
    async def test_unary_typed_roundtrip(self) -> None:
        body = serialize(
            _fbthrift_EchoService_echo_result(success="HELLO"),
            protocol=Protocol.COMPACT,
        )
        handler = _CannedHandler(_compact_reply("echo", body))
        async with get_bridged_async_client(EchoService, handler) as client:
            result = await client.echo("hi")
        self.assertEqual(result, "HELLO")
        request = handler.requests[0][0]
        self.assertEqual(_compact_method(request), "echo")
        self.assertEqual(
            _compact_body(request),
            serialize(
                _fbthrift_EchoService_echo_args(input="hi"),
                protocol=Protocol.COMPACT,
            ),
        )

    async def test_oneway_fire_and_forget(self) -> None:
        handler = _CannedHandler()
        async with get_bridged_async_client(TestService, handler) as client:
            await client.oneway()
            # oneway returns before the fire-and-forget send task runs; wait for
            # the handler to actually record the request rather than guessing at
            # how many event-loop turns the chain takes.
            await asyncio.wait_for(handler.called.wait(), timeout=5)
        self.assertEqual(len(handler.requests), 1)
        self.assertEqual(_compact_method(handler.requests[0][0]), "oneway")

    async def test_transport_error_keeps_its_type_and_message_unlogged(self) -> None:
        handler = _RaisingHandler(
            TransportError(
                TransportErrorType.TIMED_OUT,
                "Timed out after 500 ms",
                0,
                TransportOptions(0),
            )
        )
        with self.assertNoLogs(_LOGGER):
            async with get_bridged_async_client(EchoService, handler) as client:
                with self.assertRaises(TransportError) as ctx:
                    await client.echo("hi")
        self.assertEqual(ctx.exception.type, TransportErrorType.TIMED_OUT)
        self.assertEqual(ctx.exception.message, "Timed out after 500 ms")

    async def test_transport_error_subclass_arrives_as_transport_error(self) -> None:
        handler = _RaisingHandler(
            _SubclassedTransportError(
                TransportErrorType.UNKNOWN,
                "ServiceRouterError: no hosts for my.tier",
                0,
                TransportOptions(0),
            )
        )
        with self.assertNoLogs(_LOGGER):
            async with get_bridged_async_client(EchoService, handler) as client:
                with self.assertRaises(TransportError) as ctx:
                    await client.echo("hi")
        self.assertIs(type(ctx.exception), TransportError)
        self.assertEqual(ctx.exception.type, TransportErrorType.UNKNOWN)
        self.assertEqual(
            ctx.exception.message, "ServiceRouterError: no hosts for my.tier"
        )

    async def test_other_exception_surfaces_only_its_class_name(self) -> None:
        handler = _RaisingHandler(ValueError("sensitive detail"))
        with self.assertLogs(_LOGGER, level="ERROR") as logs:
            async with get_bridged_async_client(EchoService, handler) as client:
                with self.assertRaises(ApplicationError) as ctx:
                    await client.echo("hi")
        self.assertEqual(ctx.exception.type, ApplicationErrorType.UNKNOWN)
        self.assertEqual(ctx.exception.message, "ValueError")
        self.assertIn("glue bridge ChannelHandler.send_request failed", logs.output[0])

    async def test_unconvertible_transport_error_falls_back_to_class_name(
        self,
    ) -> None:
        # No args, so `.type` raises: the bridge must still complete the promise.
        handler = _RaisingHandler(TransportError.__new__(TransportError))
        with self.assertLogs(_LOGGER, level="ERROR") as logs:
            async with get_bridged_async_client(EchoService, handler) as client:
                with self.assertRaises(ApplicationError) as ctx:
                    await client.echo("hi")
        self.assertEqual(ctx.exception.message, "TransportError")
        self.assertIn("glue bridge ChannelHandler.send_request failed", logs.output[0])

    async def test_transport_error_with_an_invalid_type_falls_back_without_hanging(
        self,
    ) -> None:
        # The client could not map 999 back to a TransportErrorType, so
        # forwarding it would leave the call waiting forever.
        handler = _RaisingHandler(
            TransportError.__new__(
                TransportError,
                _NotATransportErrorType.BOGUS,
                "bad type",
                0,
                TransportOptions(0),
            )
        )
        with self.assertLogs(_LOGGER, level="ERROR"):
            async with get_bridged_async_client(EchoService, handler) as client:
                with self.assertRaises(ApplicationError) as ctx:
                    await asyncio.wait_for(client.echo("hi"), timeout=5)
        self.assertEqual(ctx.exception.message, "TransportError")

    async def test_transport_error_drops_errno_options_and_extra_args(self) -> None:
        handler = _RaisingHandler(
            TransportError(
                TransportErrorType.NOT_OPEN,
                "Connection refused",
                111,
                TransportOptions.CHANNEL_IS_VALID,
                "extra",
            )
        )
        async with get_bridged_async_client(EchoService, handler) as client:
            with self.assertRaises(TransportError) as ctx:
                await client.echo("hi")
        self.assertEqual(ctx.exception.errno, 0)
        self.assertEqual(ctx.exception.options, TransportOptions(0))
        self.assertEqual(
            ctx.exception.args,
            (TransportErrorType.NOT_OPEN, "Connection refused", 0, TransportOptions(0)),
        )

    async def test_base_exception_completes_the_promise_with_its_class_name(
        self,
    ) -> None:
        handler = _RaisingHandler(asyncio.CancelledError())
        with self.assertNoLogs(_LOGGER):
            async with get_bridged_async_client(EchoService, handler) as client:
                with self.assertRaises(ApplicationError) as ctx:
                    await client.echo("hi")
        self.assertEqual(ctx.exception.type, ApplicationErrorType.UNKNOWN)
        self.assertEqual(ctx.exception.message, "CancelledError")
