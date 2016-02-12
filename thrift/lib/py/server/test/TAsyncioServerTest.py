# @lint-avoid-pyflakes2
# @lint-avoid-python-3-compatibility-imports

import asyncio
import functools
import socket
import threading
import unittest

from thrift_asyncio.tutorial import Calculator
from thrift_asyncio.sleep import Sleep
from thrift.util.asyncio import create_client
from thrift.server.TAsyncioServer import (
    ThriftAsyncServerFactory,
    ThriftClientProtocolFactory,
)
from thrift.server.test.handler import (
    AsyncCalculatorHandler,
    AsyncSleepHandler,
)


def server_loop_runner(loop, sock, handler):
    return loop.run_until_complete(
        ThriftAsyncServerFactory(handler, port=None, loop=loop, sock=sock),
    )


@asyncio.coroutine
def test_server_with_client(sock, loop):
    port = sock.getsockname()[1]
    (transport, protocol) = yield from loop.create_connection(
        ThriftClientProtocolFactory(Calculator.Client, loop=loop),
        host='localhost',
        port=port,
    )
    client = protocol.client
    add_result = yield from asyncio.wait_for(
        client.add(1, 2),
        timeout=None,
        loop=loop,
    )
    transport.close()
    protocol.close()
    return add_result


class TAsyncioServerTest(unittest.TestCase):

    def _test_using_event_loop(self, loop):
        sock = socket.socket()
        server_loop_runner(loop, sock, AsyncCalculatorHandler())
        add_result = loop.run_until_complete(
            test_server_with_client(sock, loop)
        )
        self.assertEqual(42, add_result)

    def test_default_event_loop(self):
        loop = asyncio.get_event_loop()
        loop.set_debug(True)
        self._test_using_event_loop(loop)

    def test_custom_event_loop(self):
        loop = asyncio.new_event_loop()
        loop.set_debug(True)
        self.assertIsNot(loop, asyncio.get_event_loop())
        self._test_using_event_loop(loop)

    def _start_server_thread(self, server, loop):

        def _run(server, loop):
            loop.run_until_complete(server.wait_closed())
        t = threading.Thread(target=functools.partial(_run, server, loop))
        t.start()
        return t

    def test_server_in_separate_thread(self):
        sock = socket.socket()
        server_loop = asyncio.new_event_loop()
        server_loop.set_debug(True)
        server = server_loop_runner(server_loop, sock, AsyncCalculatorHandler())
        server_thread = self._start_server_thread(server, server_loop)

        client_loop = asyncio.new_event_loop()
        client_loop.set_debug(True)
        add_result = client_loop.run_until_complete(
            test_server_with_client(sock, client_loop),
        )
        self.assertEqual(42, add_result)

        server_loop.call_soon_threadsafe(server.close)
        server_thread.join()

    @asyncio.coroutine
    def _make_out_of_order_calls(self, sock, loop):
        port = sock.getsockname()[1]
        client_manager = yield from create_client(
            Sleep.Client,
            host='localhost',
            port=port,
            loop=loop,
        )
        with client_manager as client:
            futures = [
                client.echo(str(delay), delay * .1)
                for delay in [3, 2, 1]
            ]
            results_in_arrival_order = []
            for f in asyncio.as_completed(futures, loop=loop):
                result = yield from f
                results_in_arrival_order.append(result)
            self.assertEquals(['1', '2', '3'], results_in_arrival_order)

    def test_out_of_order_calls(self):
        sock = socket.socket()
        server_loop = asyncio.new_event_loop()
        server_loop.set_debug(True)
        handler = AsyncSleepHandler(server_loop)
        server = server_loop_runner(server_loop, sock, handler)
        server_thread = self._start_server_thread(server, server_loop)

        client_loop = asyncio.new_event_loop()
        client_loop.set_debug(True)
        client_loop.run_until_complete(
            self._make_out_of_order_calls(sock, client_loop),
        )

        server_loop.call_soon_threadsafe(server.close)
        server_thread.join()
