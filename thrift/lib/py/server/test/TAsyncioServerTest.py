# @lint-avoid-pyflakes2
# @lint-avoid-python-3-compatibility-imports

from thrift.server.TAsyncioServer import ThriftAsyncServerFactory
from thrift.server.TAsyncioServer import ThriftClientProtocolFactory
from tutorial import Calculator
import asyncio
import socket
import unittest


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
    asyncio.set_event_loop(loop)
    handler = DummyCalcHandler()
    loop.run_until_complete(
        ThriftAsyncServerFactory(handler, port=None, loop=loop, sock=sock),
    )


@asyncio.coroutine
def test_server_with_client(sock, loop):
    port = sock.getsockname()[1]
    (transport, protocol) = yield from loop.create_connection(
            ThriftClientProtocolFactory(Calculator.Client),
            host='localhost',
            port=port)
    client = protocol.client
    sum = yield from asyncio.wait_for(client.add(1, 2), None)
    return sum


class TAsyncioServerTest(unittest.TestCase):
    def testServerWithSockPassedIn(self):
        sock = socket.socket()
        loop = asyncio.get_event_loop()
        server_loop_runner(loop, sock)
        add_result = loop.run_until_complete(
            test_server_with_client(sock, loop)
        )
        self.assertEqual(42, add_result)
