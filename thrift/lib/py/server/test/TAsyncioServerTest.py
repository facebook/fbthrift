#!/usr/bin/env python3

import asyncio
import functools
import socket
import threading
import unittest

from contextlib import contextmanager
from thrift_asyncio.tutorial import Calculator
from thrift_asyncio.sleep import Sleep
from thrift.util.asyncio import create_client
from thrift.protocol.THeaderProtocol import THeaderProtocol
from thrift.server.TAsyncioServer import (
    ThriftAsyncServerFactory,
    ThriftClientProtocolFactory,
    ThriftHeaderClientProtocol,
)
from thrift.server.test.handler import (
    AsyncCalculatorHandler,
    AsyncSleepHandler,
)
from thrift.transport.TTransport import TTransportException
from thrift.Thrift import TApplicationException


def server_loop_runner(loop, sock, handler):
    return loop.run_until_complete(
        ThriftAsyncServerFactory(handler, port=None, loop=loop, sock=sock),
    )

@asyncio.coroutine
def test_server_with_client(sock, loop, factory=ThriftClientProtocolFactory):
    port = sock.getsockname()[1]
    (transport, protocol) = yield from loop.create_connection(
        factory(Calculator.Client, loop=loop),
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


@asyncio.coroutine
def test_echo_timeout(sock, loop, factory=ThriftClientProtocolFactory):
    port = sock.getsockname()[1]
    (transport, protocol) = yield from loop.create_connection(
        factory(Sleep.Client, loop=loop, timeouts={'echo': 1}),
        host='localhost',
        port=port,
    )
    client = protocol.client
    # Ask the server to delay for 30 seconds.
    # However, we told the client factory above to use a 1 second timeout
    # for the echo() function.
    yield from asyncio.wait_for(
        client.echo('test', 30),
        timeout=None,
        loop=loop,
    )
    transport.close()
    protocol.close()


class TestTHeaderProtocol(THeaderProtocol):

    def __init__(self, probe, *args, **kwargs):
        THeaderProtocol.__init__(self, *args, **kwargs)
        self.probe = probe

    def readMessageBegin(self):
        self.probe.touch()
        return THeaderProtocol.readMessageBegin(self)


class TestTHeaderProtocolFactory(object):

    def __init__(self, probe, *args, **kwargs):
        self.probe = probe
        self.args = args
        self.kwargs = kwargs

    def getProtocol(self, trans):
        return TestTHeaderProtocol(
            self.probe,
            trans,
            *self.args,
            **self.kwargs,
        )


class TestThriftClientProtocol(ThriftHeaderClientProtocol):
    THEADER_PROTOCOL_FACTORY = None

    def __init__(self, probe, *args, **kwargs):
        ThriftHeaderClientProtocol.__init__(self, *args, **kwargs)

        def factory(*args, **kwargs):
            return TestTHeaderProtocolFactory(probe, *args, **kwargs)

        self.THEADER_PROTOCOL_FACTORY = factory


class TAsyncioServerTest(unittest.TestCase):

    def test_THEADER_PROTOCOL_FACTORY_readMessageBegin(self):
        loop = asyncio.get_event_loop()
        loop.set_debug(True)
        sock = socket.socket()
        server_loop_runner(loop, sock, AsyncCalculatorHandler())

        class Probe(object):
            def __init__(self):
                self.touched = False

            def touch(self):
                self.touched = True

        probe = Probe()

        def factory(*args, **kwargs):
            return functools.partial(
                TestThriftClientProtocol,
                probe,
                *args,
                **kwargs,
            )

        add_result = loop.run_until_complete(
            test_server_with_client(
                sock,
                loop,
                factory=factory,
            )
        )
        self.assertTrue(probe.touched)
        self.assertEqual(42, add_result)

    def test_read_error(self):
        '''Test the behavior if readMessageBegin() throws an exception'''
        loop = asyncio.get_event_loop()
        loop.set_debug(True)
        sock = socket.socket()
        server_loop_runner(loop, sock, AsyncCalculatorHandler())

        # A helper Probe class that will raise an exception when
        # it is invoked by readMessageBegin()
        class Probe(object):
            def touch(self):
                raise TTransportException(
                    TTransportException.INVALID_TRANSFORM,
                    'oh noes')

        probe = Probe()

        def factory(*args, **kwargs):
            return functools.partial(
                TestThriftClientProtocol,
                probe,
                *args,
                **kwargs,
            )

        try:
            add_result = loop.run_until_complete(
                test_server_with_client(
                    sock,
                    loop,
                    factory=factory,
                )
            )
            self.fail('expected client method to throw; instead returned %r' %
                      (add_result,))
        except TTransportException as ex:
            self.assertEqual(str(ex), 'oh noes')
            self.assertEqual(ex.type, TTransportException.INVALID_TRANSFORM)

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

    @contextmanager
    def server_in_background_thread(self, sock):
        server_loop = asyncio.new_event_loop()
        server_loop.set_debug(True)
        handler = AsyncSleepHandler(server_loop)
        server = server_loop_runner(server_loop, sock, handler)
        server_thread = self._start_server_thread(server, server_loop)
        try:
            yield server
        finally:
            server_loop.call_soon_threadsafe(server.close)
            server_thread.join()

    def test_out_of_order_calls(self):
        sock = socket.socket()
        with self.server_in_background_thread(sock):
            client_loop = asyncio.new_event_loop()
            client_loop.set_debug(True)
            client_loop.run_until_complete(
                self._make_out_of_order_calls(sock, client_loop),
            )

    @asyncio.coroutine
    def _assert_transport_is_closed_on_error(self, sock, loop):
        port = sock.getsockname()[1]
        client_manager = yield from create_client(
            Sleep.Client,
            host='localhost',
            port=port,
            loop=loop,
        )
        try:
            with client_manager as client:
                raise Exception('expected exception from test')
        except Exception:
            self.assertFalse(client._oprot.trans.isOpen())

    def test_close_client_on_error(self):
        sock = socket.socket()
        with self.server_in_background_thread(sock):
            loop = asyncio.new_event_loop()
            loop.set_debug(True)
            loop.run_until_complete(
                self._assert_transport_is_closed_on_error(sock, loop),
            )

    def test_timeout(self):
        loop = asyncio.get_event_loop()
        loop.set_debug(True)
        sock = socket.socket()
        server_loop_runner(loop, sock, AsyncSleepHandler(loop))
        with self.assertRaisesRegex(
            TApplicationException, 'Call to echo timed out'
        ):
            loop.run_until_complete(test_echo_timeout(sock, loop))
