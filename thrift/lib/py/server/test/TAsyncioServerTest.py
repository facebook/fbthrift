# @lint-avoid-pyflakes2
# @lint-avoid-python-3-compatibility-imports

import asyncio
import functools
import socket
import threading
import unittest

from thrift.server.TAsyncioServer import ThriftAsyncServerFactory
from thrift.server.TAsyncioServer import ThriftClientProtocolFactory
from tutorial import Calculator


class DummyCalcHandler(Calculator.Iface):
    @asyncio.coroutine
    def add(self, n1, n2):
        return 42

    @asyncio.coroutine
    def calculate(self, logid, work):
        return 0

    @asyncio.coroutine
    def zip(self):
        print('zip')


def server_loop_runner(loop, sock):
    handler = DummyCalcHandler()
    return loop.run_until_complete(
        ThriftAsyncServerFactory(handler, port=None, loop=loop, sock=sock),
    )


@asyncio.coroutine
def test_server_with_client(sock, loop):
    port = sock.getsockname()[1]
    (transport, protocol) = yield from loop.create_connection(
        ThriftClientProtocolFactory(Calculator.Client, loop),
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
        server_loop_runner(loop, sock)
        add_result = loop.run_until_complete(
            test_server_with_client(sock, loop)
        )
        self.assertEqual(42, add_result)

    def test_default_event_loop(self):
        loop = asyncio.get_event_loop()
        self._test_using_event_loop(loop)

    def test_custom_event_loop(self):
        loop = asyncio.new_event_loop()
        self.assertIsNot(loop, asyncio.get_event_loop())
        self._test_using_event_loop(loop)

    def _start_server_thread(self, loop, sock):
        server = server_loop_runner(loop, sock)

        def _run(server, loop):
            loop.run_until_complete(server.wait_closed())
        t = threading.Thread(target=functools.partial(_run, server, loop))
        t.start()
        return server, t

    def test_server_in_separate_thread(self):
        sock = socket.socket()
        server_loop = asyncio.new_event_loop()
        server_loop.set_debug(True)
        server, server_thread = self._start_server_thread(server_loop, sock)

        client_loop = asyncio.new_event_loop()
        client_loop.set_debug(True)
        add_result = client_loop.run_until_complete(
            test_server_with_client(sock, client_loop),
        )
        self.assertEqual(42, add_result)

        server_loop.call_soon_threadsafe(server.close)
        server_thread.join()
