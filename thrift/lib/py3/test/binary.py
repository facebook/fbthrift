#!/usr/bin/env python3
import asyncio
import unittest
from typing import Any

from folly.iobuf import IOBuf
from thrift.py3 import get_client, ThriftServer
import thrift.py3.server

from binary.clients import BinaryService
from binary.services import BinaryServiceInterface
from binary.types import Binaries


class BinaryTests(unittest.TestCase):
    def test_various_binary_types(self) -> None:
        val = Binaries(
            no_special_type=b'abcdef',
            iobuf_val=IOBuf(b'mnopqr'),
            iobuf_ptr=IOBuf(b'ghijkl'),
            fbstring=b'stuvwx',
            nonstandard_type=b'yzabcd',
        )
        self.assertEqual(val.no_special_type, b'abcdef')
        self.assertEqual(bytes(val.iobuf_val), b'mnopqr')
        assert val.iobuf_ptr is not None
        self.assertEqual(bytes(val.iobuf_ptr), b'ghijkl')
        self.assertEqual(val.fbstring, b'stuvwx')
        self.assertEqual(val.nonstandard_type, b'yzabcd')


class BinaryHandler(BinaryServiceInterface):
    def __init__(self, unit_test):
        self.unit_test = unit_test

    async def sendRecvBinaries(self, val: Binaries) -> Binaries:
        self.unit_test.assertEqual(val.no_special_type, b'c1')
        self.unit_test.assertEqual(bytes(val.iobuf_val), b'c2')
        assert val.iobuf_ptr is not None
        self.unit_test.assertEqual(bytes(val.iobuf_ptr), b'c3')
        self.unit_test.assertEqual(val.fbstring, b'c4')
        self.unit_test.assertEqual(val.nonstandard_type, b'c5')
        return Binaries(
            no_special_type=b's1',
            iobuf_val=IOBuf(b's2'),
            iobuf_ptr=IOBuf(b's3'),
            fbstring=b's4',
            nonstandard_type=b's5',
        )

    async def sendRecvBinary(self, val: bytes) -> bytes:
        self.unit_test.assertEqual(val, b'cv1')
        return b'sv1'

    async def sendRecvIOBuf(self, val: IOBuf) -> IOBuf:
        self.unit_test.assertEqual(bytes(val), b'cv2')
        return IOBuf(b'sv2')

    async def sendRecvIOBufPtr(self, val: IOBuf) -> IOBuf:
        self.unit_test.assertEqual(bytes(val), b'cv3')
        return IOBuf(b'sv3')

    async def sendRecvFbstring(self, val: bytes) -> bytes:
        self.unit_test.assertEqual(val, b'cv4')
        return b'sv4'

    async def sendRecvBuffer(self, val: bytes) -> bytes:
        self.unit_test.assertEqual(val, b'cv5')
        return b'sv5'


class TestServer:
    server: ThriftServer
    serve_task: asyncio.Task

    def __init__(
        self,
        *,
        ip: str,
        handler: thrift.py3.server.ServiceInterface,
    ) -> None:
        self.server = ThriftServer(handler, ip=ip, path=None)

    async def __aenter__(self) -> thrift.py3.server.SocketAddress:
        self.serve_task = asyncio.get_event_loop().create_task(self.server.serve())
        return await self.server.get_address()

    async def __aexit__(self, *exc_info) -> None:
        self.server.stop()
        await self.serve_task


class ClientBinaryServerTests(unittest.TestCase):
    def test_send_recv(self) -> None:
        loop = asyncio.get_event_loop()

        async def inner_test() -> None:
            async with TestServer(handler=BinaryHandler(self), ip="::1") as sa:
                assert sa.ip and sa.port
                async with get_client(
                        BinaryService, host=sa.ip, port=sa.port) as client:
                    val : Any
                    val = await client.sendRecvBinaries(Binaries(
                        no_special_type=b'c1',
                        iobuf_val=IOBuf(b'c2'),
                        iobuf_ptr=IOBuf(b'c3'),
                        fbstring=b'c4',
                        nonstandard_type=b'c5',
                    ))
                    self.assertEqual(val.no_special_type, b's1')
                    self.assertEqual(bytes(val.iobuf_val), b's2')
                    assert val.iobuf_ptr is not None
                    self.assertEqual(bytes(val.iobuf_ptr), b's3')
                    self.assertEqual(val.fbstring, b's4')
                    self.assertEqual(val.nonstandard_type, b's5')

                    val = await client.sendRecvBinary(b'cv1')
                    self.assertEqual(val, b'sv1')

                    val = await client.sendRecvIOBuf(IOBuf(b'cv2'))
                    self.assertEqual(bytes(val), b'sv2')

                    val = await client.sendRecvIOBufPtr(IOBuf(b'cv3'))
                    self.assertEqual(bytes(val), b'sv3')

                    val = await client.sendRecvFbstring(b'cv4')
                    self.assertEqual(val, b'sv4')

                    val = await client.sendRecvBuffer(b'cv5')
                    self.assertEqual(val, b'sv5')

        loop.run_until_complete(inner_test())
