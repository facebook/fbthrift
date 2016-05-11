#!/usr/bin/env python2

import functools
import logging
import time
import unittest

from ThriftTest import ThriftTest
from ThriftTest.ttypes import Xception, Xtruct
from thrift.server.TTrolliusServer import (
    ThriftClientProtocolFactory,
    ThriftAsyncServerFactory,
)
from thrift.transport.THeaderTransport import THeaderTransport

import trollius as asyncio
from trollius import From, Return

loop = asyncio.get_event_loop()
loop.set_debug(True)
logging.getLogger('asyncio').setLevel(logging.DEBUG)


class TestHandler(ThriftTest.Iface):

    def __init__(self, use_async=False):
        self.onewaysQueue = asyncio.Queue(loop=loop)

    def testVoid(self):
        pass

    @asyncio.coroutine
    def testString(self, s):
        yield From(asyncio.sleep(0))
        raise Return(s)

    def testByte(self, b):
        return b

    def testI16(self, i16):
        return i16

    def testI32(self, i32):
        return i32

    def testI64(self, i64):
        return i64

    def testDouble(self, dub):
        return dub

    def testStruct(self, thing):
        return thing

    def testException(self, s):
        if s == 'Xception':
            x = Xception()
            x.errorCode = 1001
            x.message = s
            raise x
        elif s == "throw_undeclared":
            raise ValueError("foo")

    @asyncio.coroutine
    def testOneway(self, seconds):
        t = time.time()
        yield From(asyncio.sleep(seconds))
        yield From(self.onewaysQueue.put((t, time.time(), seconds)))

    def testNest(self, thing):
        return thing

    def testMap(self, thing):
        return thing

    def testSet(self, thing):
        return thing

    def testList(self, thing):
        return thing

    def testEnum(self, thing):
        return thing

    def testTypedef(self, thing):
        return thing


def async_test(fn):

    @functools.wraps(fn)
    def wrapper(*args, **kwargs):
        f = fn if asyncio.iscoroutinefunction(fn) else asyncio.coroutine(fn)
        loop.run_until_complete(f(*args, **kwargs))

    return wrapper


class ThriftCoroTestCase(unittest.TestCase):
    CLIENT_TYPE = None

    @async_test
    def setUp(self):
        global loop
        self.host = '127.0.0.1'
        self.handler = TestHandler(use_async=False)
        self.server = yield From(ThriftAsyncServerFactory(
            self.handler, interface=self.host, port=0, loop=loop,
        ))
        self.port = self.server.sockets[0].getsockname()[1]
        self.transport, self.protocol = yield From(loop.create_connection(
            ThriftClientProtocolFactory(
                ThriftTest.Client,
                client_type=self.CLIENT_TYPE),
            host=self.host,
            port=self.port,
        ))
        self.client = self.protocol.client

    @async_test
    def tearDown(self):
        self.protocol.close()
        self.transport.close()
        self.protocol.close()
        self.server.close()

    @async_test
    def testVoid(self):
        result = yield From(self.client.testVoid())
        self.assertEqual(result, None)

    @async_test
    def testString(self):
        result = yield From(self.client.testString('Python'))
        self.assertEqual(result, 'Python')

    @async_test
    def testByte(self):
        result = yield From(self.client.testByte(63))
        self.assertEqual(result, 63)

    @async_test
    def testI32(self):
        result = yield From(self.client.testI32(-1))
        self.assertEqual(result, -1)
        result = yield From(self.client.testI32(0))
        self.assertEqual(result, 0)

    @async_test
    def testI64(self):
        result = yield From(self.client.testI64(-34359738368))
        self.assertEqual(result, -34359738368)

    @async_test
    def testDouble(self):
        result = yield From(self.client.testDouble(-5.235098235))
        self.assertAlmostEqual(result, -5.235098235)

    @async_test
    def testStruct(self):
        x = Xtruct()
        x.string_thing = "Zero"
        x.byte_thing = 1
        x.i32_thing = -3
        x.i64_thing = -5
        y = yield From(self.client.testStruct(x))

        self.assertEqual(y.string_thing, "Zero")
        self.assertEqual(y.byte_thing, 1)
        self.assertEqual(y.i32_thing, -3)
        self.assertEqual(y.i64_thing, -5)

    @async_test
    def testException(self):
        yield From(self.client.testException('Safe'))
        try:
            yield From(self.client.testException('Xception'))
            self.fail("Xception not raised")
        except Xception as x:
            self.assertEqual(x.errorCode, 1001)
            self.assertEqual(x.message, 'Xception')

        try:
            yield From(self.client.testException("throw_undeclared"))
            self.fail("exception not raised")
        except Exception:  # type is undefined
            pass

    @async_test
    def testOneway(self):
        yield From(self.client.testOneway(2))
        start, end, seconds = yield From(self.handler.onewaysQueue.get())
        self.assertAlmostEqual(seconds, (end - start), places=1)

    @async_test
    def testClose(self):
        self.assertTrue(self.protocol.transport.isOpen())
        self.protocol.close()
        self.assertFalse(self.protocol.transport.isOpen())


class FramedThriftCoroTestCase(ThriftCoroTestCase):
    CLIENT_TYPE = THeaderTransport.FRAMED_DEPRECATED
