#!/usr/bin/env python3

from unittest.mock import MagicMock
from libfb.py.asyncio.unittest import TestCase

from thrift.server.TAsyncioServer import ThriftAsyncServerFactory
from thrift.Thrift import TProcessor


class TestAsyncioServer(TestCase):
    async def test_factory(self):
        async def wrap(value):
            return value

        protocol_factory = MagicMock()
        loop = MagicMock()
        processor = MagicMock(spec=TProcessor)
        event_handler = MagicMock()
        server = MagicMock()
        sock = MagicMock()
        sock.getsockname.return_value = "foosock"
        server.sockets = [sock]
        loop.create_server.return_value = wrap(server)

        await ThriftAsyncServerFactory(
            processor,
            loop=loop,
            event_handler=event_handler,
            protocol_factory=protocol_factory,
        )

        event_handler.preServe.assert_called_with("foosock")
