#!/usr/bin/env python

# This is loosely based on RunClientServer.py and TestClient.py, but
# is heavily modified to work in fbcode.  In particular, all the assertions
# are borrowed from testClient.py
#
# This starts up a bunch of servers, one for each of the server type
# and socket typecombinations we have. It then runs through the tests
# for each server, which entails connecting to calling a method on the
# server, asserting something about that method, and then closing the
# connection

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from itertools import chain
import os.path
import getpass
import sys
from subprocess import Popen, PIPE, STDOUT
import time
import unittest
import unittest2
import string
import socket
import errno
import ssl as SSL

from ThriftTest import ThriftTest
from ThriftTest.ttypes import *

from thrift.transport import TTransport
from thrift.transport.THeaderTransport import THeaderTransport
from thrift.transport import TSocket, TSSLSocket
from thrift.protocol import TBinaryProtocol, THeaderProtocol


SERVER_TYPES = [
    "TForkingServer",
    "TThreadPoolServer",
    "TThreadedServer",
    "TSimpleServer",
    "TNonblockingServer",
    ]
FRAMED_TYPES = ["TNonblockingServer"]
_servers = []
_ports = {}


def start_server(server_type, ssl, server_header, server_context, port):
    server_path = '_bin/thrift/test/py/python_test_server.par'
    args = [server_path, '--port', str(port)]
    if ssl:
        args.append('--ssl')
    if server_header:
        args.append('--header')
    if server_type == "TNonblockingServer":
        args.append('--timeout=1')
    if server_context:
        args.append('--context')
    args.append(server_type)
    stdout = None
    stderr = None
    if sys.stdout.isatty():
        stdout = sys.stdout
        stderr = sys.stderr
    return Popen(args, stdout=stdout, stderr=stderr)


def wait_for_server(port, timeout, ssl=False):
    end = time.time() + timeout
    while time.time() < end:
        try:
            sock = socket.socket()
            if ssl:
                sock = SSL.wrap_socket(sock)
            sock.connect(('localhost', port))
            sock.close()
            return True
        except socket.timeout:
            return False
        except socket.error as e:
            if e[0] != errno.ECONNREFUSED:
                raise
            time.sleep(0.1)
            continue
    return False


class AbstractTest(object):

    @classmethod
    def setUpClass(cls):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(
            socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind(('0.0.0.0', 0))
        port = sock.getsockname()[1]
        server = start_server(
            cls.server_type,
            cls.ssl,
            cls.server_header,
            cls.server_context,
            port)

        if not wait_for_server(port, 5.0, ssl=cls.ssl):
            msg = "Failed to start " + cls.server_type
            if cls.ssl:
                msg += " using ssl"
            if cls.server_header:
                msg += " using header protocol"
            if cls.server_context:
                msg += " using context"
            raise Exception(msg)

        cls._port = port
        cls._server = server

    @classmethod
    def tearDownClass(cls):
        cls._server.kill()
        cls._server.wait()

    def setUp(self):
        if self.ssl:
            self.socket = TSSLSocket.TSSLSocket("localhost", self._port)
        else:
            self.socket = TSocket.TSocket("localhost", self._port)

        if self.server_type in FRAMED_TYPES \
                and not isinstance(self, HeaderTest):
            self.transport = TTransport.TFramedTransport(self.socket)
        else:
            self.transport = TTransport.TBufferedTransport(self.socket)

        self.protocol = self.protocol_factory.getProtocol(self.transport)
        self.transport.open()
        self.client = ThriftTest.Client(self.protocol)

    def tearDown(self):
        self.transport.close()

    def testVoid(self):
        self.client.testVoid()

    def testString(self):
        self.assertEqual(self.client.testString('Python'), 'Python')

    def testByte(self):
        self.assertEqual(self.client.testByte(63), 63)

    def testI32(self):
        self.assertEqual(self.client.testI32(-1), -1)
        self.assertEqual(self.client.testI32(0), 0)

    def testI64(self):
        self.assertEqual(self.client.testI64(-34359738368), -34359738368)

    def testDouble(self):
        self.assertEqual(self.client.testDouble(-5.235098235), -5.235098235)

    def testFloat(self):
        self.assertEqual(self.client.testFloat(-5.25), -5.25)

    def testStruct(self):
        x = Xtruct()
        x.string_thing = "Zero"
        x.byte_thing = 1
        x.i32_thing = -3
        x.i64_thing = -5
        y = self.client.testStruct(x)

        self.assertEqual(y.string_thing, "Zero")
        self.assertEqual(y.byte_thing, 1)
        self.assertEqual(y.i32_thing, -3)
        self.assertEqual(y.i64_thing, -5)

    def testException(self):
        self.client.testException('Safe')
        try:
            self.client.testException('Xception')
            self.fail("should have gotten exception")
        except Xception as x:
            self.assertEqual(x.errorCode, 1001)
            self.assertEqual(x.message, 'Xception')

        try:
            self.client.testException("throw_undeclared")
            self.fail("should have thrown exception")
        except Exception:  # type is undefined
            pass

    def testOneway(self):
        start = time.time()
        self.client.testOneway(1)
        end = time.time()
        self.assertTrue(end - start < 0.2,
                        "oneway sleep took %f sec" % (end - start))

    def testPreServe(self):
        count = self.client.testPreServe()
        self.assertGreaterEqual(count, 1)

    def testNewConnection(self):
        count = self.client.testNewConnection()
        self.assertTrue(count > 0)

    def testRequestCount(self):
        count = self.client.testRequestCount()
        # not updated for TNonblockingServer
        self.assertTrue(count >= 0)

    def testConnectionDestroyed(self):
        count = self.client.testConnectionDestroyed()
        self.assertTrue(count >= 0)

    def testEventCountRelationships(self):
        orig_num_pre_serve = self.client.testPreServe()
        orig_num_new = self.client.testNewConnection()
        orig_num_dest = self.client.testConnectionDestroyed()

        for i in xrange(5):
            # force new server connection
            self.transport.getTransport().close()
            self.transport.getTransport().open()

            num_pre_serve = self.client.testPreServe()
            num_new = self.client.testNewConnection()
            num_dest = self.client.testConnectionDestroyed()

            # always exactly one pre-serve event
            self.assertGreaterEqual(num_pre_serve, 1)

            # every iteration is a new re-connection
            self.assertEquals(num_new, orig_num_new + i + 1)

            # every iteration is a new connection destroyed, and
            # current conneciton is active, so num_dest should be
            # num_new - 1.  Except:
            # 1) TForkingServer does not update num_dest (or rather,
            #    the num_dest is updated in the child process, which
            #    doesn't make it back to the parent process), so
            #    num_dest is always 0 in this case
            # 2) TNonblockingServer fires the connectionDestroyed()
            #    event on the top of the next handle() iteration, so
            #    the count could be off by 2 in this case rather than 1.
            # 3) THeaderProtocol on the server breaks this for some reason
            if num_dest and not self.server_header:
                self.assert_(num_new - num_dest <= 2)

    def testNonblockingTimeout(self):
        if self.server_type == "TNonblockingServer":
            self.socket.close()
            self.socket.open()
            stime = time.time()
            try:
                self.socket.read(1)
            except TTransport.TTransportException as x:
                total_time = time.time() - stime
                self.assertTrue(total_time > 1, "Read timeout was too short")
                self.assertTrue(total_time < 10, "Read timeout took too long")
                return
            self.assertTrue(False, "Read timeout never fired")


class NormalBinaryTest(AbstractTest):
    protocol_factory = TBinaryProtocol.TBinaryProtocolFactory()


class AcceleratedBinaryTest(AbstractTest):
    protocol_factory = TBinaryProtocol.TBinaryProtocolAcceleratedFactory()


class HeaderTest(AbstractTest):
    protocol_factory = THeaderProtocol.THeaderProtocolFactory(True,
        [THeaderTransport.HEADERS_CLIENT_TYPE,
         THeaderTransport.FRAMED_DEPRECATED,
         THeaderTransport.UNFRAMED_DEPRECATED,
         THeaderTransport.HTTP_CLIENT_TYPE])

    def testZlibCompression(self):
        htrans = self.protocol.trans
        if isinstance(htrans, THeaderTransport):
            htrans.add_transform(THeaderTransport.ZLIB_TRANSFORM)
            self.testStruct()

    def testSnappyCompression(self):
        htrans = self.protocol.trans
        if isinstance(htrans, THeaderTransport):
            htrans.add_transform(THeaderTransport.SNAPPY_TRANSFORM)
            self.testStruct()

    def testMultipleCompression(self):
        htrans = self.protocol.trans
        if isinstance(htrans, THeaderTransport):
            htrans.add_transform(THeaderTransport.ZLIB_TRANSFORM)
            htrans.add_transform(THeaderTransport.SNAPPY_TRANSFORM)
            self.testStruct()

    def testKeyValueHeader(self):
        # TODO(davejwatson) - Task #2488109
        if self.server_header and self.server_type == 'TNonblockingServer':
            raise unittest.SkipTest(
                "testKeyValueHeader fails for TNonblockingServer (#2488109)")
        htrans = self.protocol.trans
        if isinstance(htrans, THeaderTransport):
            # Try just persistent header
            htrans.set_persistent_header("permanent", "true")
            self.client.testString('test')
            headers = htrans.get_headers()
            self.assertTrue("permanent" in headers)
            self.assertEquals(headers["permanent"], "true")

            # Try with two transient headers
            htrans.set_header("transient1", "true")
            htrans.set_header("transient2", "true")
            self.client.testString('test')
            headers = htrans.get_headers()
            self.assertTrue("permanent" in headers)
            self.assertEquals(headers["permanent"], "true")
            self.assertTrue("transient1" in headers)
            self.assertEquals(headers["transient1"], "true")
            self.assertTrue("transient2" in headers)
            self.assertEquals(headers["transient2"], "true")

            # Add one, update one and delete one transient header
            htrans.set_header("transient2", "false")
            htrans.set_header("transient3", "true")
            self.client.testString('test')
            headers = htrans.get_headers()
            self.assertTrue("permanent" in headers)
            self.assertEquals(headers["permanent"], "true")
            self.assertTrue("transient1" not in headers)
            self.assertTrue("transient2" in headers)
            self.assertEquals(headers["transient2"], "false")
            self.assertTrue("transient3" in headers)
            self.assertEquals(headers["transient3"], "true")


def camelcase(s):
    if not s[0].isupper():
        s = ''.join(map(string.capitalize, s.split('_')))
    return s


def class_name_mixin(k, v):
    mixin = camelcase(k)
    if isinstance(v, bool):
        mixin += 'On' if v else 'Off'
    else:
        mixin += camelcase(v)
    return mixin


def new_test_class(cls, vars):
    class Subclass(cls, unittest.TestCase):
        pass
    name = cls.__name__
    for k, v in vars.iteritems():
        name += class_name_mixin(k, v)
        setattr(Subclass, k, v)
    Subclass.__name__ = name.encode('ascii')
    return Subclass


def add_test_classes(module):
    classes = []
    for server_type in SERVER_TYPES:
        for ssl in (True, False):
            if ssl and server_type in FRAMED_TYPES:
                continue
            for server_header in (True, False):
                for server_context in (True, False):
                    vars = {
                        'server_type': server_type,
                        'ssl': ssl,
                        'server_header': server_header,
                        'server_context': server_context,
                    }
                    classes.append(new_test_class(NormalBinaryTest, vars))
                    classes.append(new_test_class(AcceleratedBinaryTest, vars))
                    # header client to non-header server hangs
                    if server_header:
                        classes.append(new_test_class(HeaderTest, vars))

    for cls in classes:
        setattr(module, cls.__name__, cls)


add_test_classes(sys.modules[__name__])
