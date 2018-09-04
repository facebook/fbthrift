#!/usr/bin/env python3

from unittest.mock import patch, MagicMock
from libfb.py.asyncio.unittest import TestCase

from thrift.server.TAsyncioServer import ThriftAsyncServerFactory
from thrift.Thrift import TProcessor


class TestAsyncioServer(TestCase):
    async def test_factory(self):
        async def wrap(value):
            return value

        with patch(
            "thrift.server.TAsyncioServer.ThriftServerProtocolFactory"
        ) as protocol_factory:
            loop = MagicMock()
            processor = MagicMock(spec=TProcessor)
            event_handler = MagicMock()
            server = MagicMock()
            sock = MagicMock()
            sock.getsockname.return_value = "foosock"
            server.sockets = [sock]
            # Weird custom iterator object in order to inject the return value
            # of the "yield from" statement in ThriftAsyncServerFactory.
            loop.create_server.return_value = wrap(server)

            await ThriftAsyncServerFactory(
                processor, loop=loop, event_handler=event_handler
            )

            protocol_factory.assert_called_once_with(processor, event_handler, loop)
            event_handler.preServe.assert_called_with("foosock")
