# @lint-avoid-pyflakes2
# @lint-avoid-python-3-compatibility-imports

import asyncio
import functools
import time
import unittest

from ThriftTest import ThriftTest
from ThriftTest.ttypes import Xception, Xtruct
from thrift.server.TAsyncioServer import (
    ThriftClientProtocolFactory,
    ThriftAsyncServerFactory,
)


loop = asyncio.get_event_loop()


class TestHandler(ThriftTest.Iface):

    def __init__(self):
        self.onewaysQueue = asyncio.Queue(loop=loop)

    def testVoid(self):
        pass

    def testString(self, s):
        return s

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

    def testOneway(self, seconds):
        @asyncio.coroutine
        def fireOneway(t):
            yield from asyncio.sleep(seconds)
            yield from self.onewaysQueue.put((t, time.time(), seconds))
        asyncio.async(fireOneway(time.time()))

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


def async_test(f):

    @functools.wraps(f)
    def wrapper(*args, **kwargs):
        global loop
        coro = asyncio.coroutine(f)
        loop.run_until_complete(coro(*args, **kwargs))

    return wrapper


class ThriftTestCase(unittest.TestCase):

    @async_test
    def setUp(self):
        global loop
        self.host = '127.0.0.1'
        self.handler = TestHandler()
        self.server = yield from ThriftAsyncServerFactory(
            self.handler, interface=self.host, port=0, loop=loop,
        )
        self.port = self.server.sockets[0].getsockname()[1]
        self.transport, self.protocol = yield from loop.create_connection(
            ThriftClientProtocolFactory(ThriftTest.Client),
            host=self.host,
            port=self.port,
        )
        self.client = self.protocol.client

    @async_test
    def tearDown(self):
        self.transport.close()
        self.server.close()

    @async_test
    def testVoid(self):
        result = yield from self.client.testVoid()
        self.assertEqual(result, None)

    @async_test
    def testString(self):
        result = yield from self.client.testString('Python')
        self.assertEqual(result, 'Python')

    @async_test
    def testByte(self):
        result = yield from self.client.testByte(63)
        self.assertEqual(result, 63)

    @async_test
    def testI32(self):
        result = yield from self.client.testI32(-1)
        self.assertEqual(result, -1)
        result = yield from self.client.testI32(0)
        self.assertEqual(result, 0)

    @async_test
    def testI64(self):
        result = yield from self.client.testI64(-34359738368)
        self.assertEqual(result, -34359738368)

    @async_test
    def testDouble(self):
        result = yield from self.client.testDouble(-5.235098235)
        self.assertAlmostEqual(result, -5.235098235)

    @async_test
    def testStruct(self):
        x = Xtruct()
        x.string_thing = "Zero"
        x.byte_thing = 1
        x.i32_thing = -3
        x.i64_thing = -5
        y = yield from self.client.testStruct(x)

        self.assertEqual(y.string_thing, "Zero")
        self.assertEqual(y.byte_thing, 1)
        self.assertEqual(y.i32_thing, -3)
        self.assertEqual(y.i64_thing, -5)

    @async_test
    def testException(self):
        yield from self.client.testException('Safe')
        try:
            yield from self.client.testException('Xception')
            self.fail("Xception not raised")
        except Xception as x:
            self.assertEqual(x.errorCode, 1001)
            self.assertEqual(x.message, 'Xception')

        try:
            yield from self.client.testException("throw_undeclared")
            self.fail("exception not raised")
        except Exception:  # type is undefined
            pass

    @async_test
    def testOneway(self):
        yield from self.client.testOneway(2)
        start, end, seconds = yield from self.handler.onewaysQueue.get()
        self.assertAlmostEqual(seconds, (end - start), places=2)
