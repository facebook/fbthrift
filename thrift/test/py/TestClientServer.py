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

import os.path
import sys
from subprocess import Popen
import time
import unittest
import string
import socket
import errno
import ssl as SSL

from ThriftTest import ThriftTest, SecondService
from ThriftTest.ttypes import *

from thrift.transport import TTransport
from thrift.transport.THeaderTransport import THeaderTransport
from thrift.transport import TSocket, TSSLSocket
from thrift.protocol import TBinaryProtocol, THeaderProtocol, \
        TMultiplexedProtocol

SERVER_TYPES = [
    "TCppServer",
    ]
if sys.version_info[0] < 3:
    SERVER_TYPES.append("TNonblockingServer")
FRAMED_TYPES = ["TNonblockingServer"]
_servers = []
_ports = {}

try:
    from thrift.protocol import fastproto
except:
    fastproto = None

def start_server(server_type, ssl, server_header, server_context,
        multiple, port):
    server_path = os.path.dirname(sys.argv[0])
    if sys.version_info[0] >= 3:
        server_bin = os.path.join(server_path, 'py3_test_server.par')
    else:
        server_bin = os.path.join(server_path, 'python_test_server.par')

    args = [server_bin, '--port', str(port)]
    if ssl:
        args.append('--ssl')
    if server_header:
        args.append('--header')
    if server_type == "TNonblockingServer":
        args.append('--timeout=1')
    if server_context:
        args.append('--context')
    if multiple:
        args.append('--multiple')
    args.append(server_type)
    stdout = None
    stderr = None
    if sys.stdout.isatty():
        stdout = sys.stdout
        stderr = sys.stderr
    return Popen(args, stdout=stdout, stderr=stderr)

def isConnectionRefused(e):
    if sys.version_info[0] >= 3:
        return isinstance(e, ConnectionRefusedError)
    else:
        return e[0] == errno.ECONNREFUSED

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
            if not isConnectionRefused(e):
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
            cls.multiple,
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

    def bytes_comp(self, seq1, seq2):
        if not isinstance(seq1, bytes):
            seq1 = seq1.encode('utf-8')
        if not isinstance(seq2, bytes):
            seq2 = seq2.encode('utf-8')
        self.assertEquals(seq1, seq2)

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
        if isinstance(self, HeaderAcceleratedCompactTest):
            self.protocol.trans.set_protocol_id(
                    THeaderProtocol.THeaderProtocol.T_COMPACT_PROTOCOL)
            self.protocol.reset_protocol()
        self.transport.open()
        self.client = ThriftTest.Client(self.protocol)
        if self.multiple:
            p = TMultiplexedProtocol.TMultiplexedProtocol(self.protocol,
                    "ThriftTest")
            self.client = ThriftTest.Client(p)
            p = TMultiplexedProtocol.TMultiplexedProtocol(self.protocol,
                    "SecondService")
            self.client2 = SecondService.Client(p)
        else:
            self.client = ThriftTest.Client(self.protocol)
            self.client2 = None


    def tearDown(self):
        self.transport.close()

    def testVoid(self):
        self.client.testVoid()

    def testString(self):
        self.bytes_comp(self.client.testString('Python'), 'Python')

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

        self.bytes_comp(y.string_thing, "Zero")
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
            self.bytes_comp(x.message, 'Xception')

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

    def testblahBlah(self):
        if self.client2:
            self.assertEqual(self.client2.blahBlah(), None)

    def testPreServe(self):
        count = self.client.testPreServe()
        self.assertGreaterEqual(count, 1)

    def testNewConnection(self):
        if self.server_type == "TCppServer":
            return
        count = self.client.testNewConnection()
        self.assertTrue(count > 0)

    def testRequestCount(self):
        count = self.client.testRequestCount()
        # not updated for TNonblockingServer
        self.assertTrue(count >= 0)

    def testConnectionDestroyed(self):
        count = self.client.testConnectionDestroyed()
        self.assertTrue(count >= 0)

    def testNonblockingTimeout(self):
        if self.server_type == "TNonblockingServer":
            self.socket.close()
            self.socket.open()
            stime = time.time()
            try:
                self.socket.read(1)
            except TTransport.TTransportException:
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
        if self.server_header and self.server_type == 'TNonblockingServer':
            # TNonblockingServer uses different protocol instances for input
            # and output so persistent header won't work
            return
        htrans = self.protocol.trans
        if isinstance(htrans, THeaderTransport):
            # Try just persistent header
            htrans.set_persistent_header(b"permanent", b"true")
            self.client.testString('test')
            headers = htrans.get_headers()
            self.assertTrue(b"permanent" in headers)
            self.assertEquals(headers[b"permanent"], b"true")

            # Try with two transient headers
            htrans.set_header(b"transient1", b"true")
            htrans.set_header(b"transient2", b"true")
            self.client.testString('test')
            headers = htrans.get_headers()
            self.assertTrue(b"permanent" in headers)
            self.assertEquals(headers[b"permanent"], b"true")
            self.assertTrue(b"transient1" in headers)
            self.assertEquals(headers[b"transient1"], b"true")
            self.assertTrue(b"transient2" in headers)
            self.assertEquals(headers[b"transient2"], b"true")

            # Add one, update one and delete one transient header
            htrans.set_header(b"transient2", b"false")
            htrans.set_header(b"transient3", b"true")
            self.client.testString('test')
            headers = htrans.get_headers()
            self.assertTrue(b"permanent" in headers)
            self.assertEquals(headers[b"permanent"], b"true")
            self.assertTrue(b"transient1" not in headers)
            self.assertTrue(b"transient2" in headers)
            self.assertEquals(headers[b"transient2"], b"false")
            self.assertTrue(b"transient3" in headers)
            self.assertEquals(headers[b"transient3"], b"true")

class HeaderAcceleratedCompactTest(AbstractTest):
    protocol_factory = THeaderProtocol.THeaderProtocolFactory(True,
        [THeaderTransport.HEADERS_CLIENT_TYPE,
         THeaderTransport.FRAMED_DEPRECATED,
         THeaderTransport.UNFRAMED_DEPRECATED,
         THeaderTransport.HTTP_CLIENT_TYPE])

def camelcase(s):
    if not s[0].isupper():
        if sys.version_info[0] >= 3:
            s = ''.join([x.capitalize() for x in s.split('_')])
        else:
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
    template = ""
    name = cls.__name__
    for k, v in sorted(vars.items()):
        name += class_name_mixin(k, v)
        template += "  {} = {!r}\n".format(k, v)
    template = "class {}(cls, unittest.TestCase):\n".format(name) + template
    exec(template)
    return locals()[name]


def add_test_classes(module):
    classes = []
    for server_type in SERVER_TYPES:
        for ssl in (True, False):
            if ssl and (server_type in FRAMED_TYPES or server_type == "TCppServer"):
                continue
            for server_header in (True, False):
                if server_header is True and server_type == "TCppServer":
                    continue
                for server_context in (True, False):
                    for multiple in (True, False):
                        vars = {
                            'server_type': server_type,
                            'ssl': ssl,
                            'server_header': server_header,
                            'server_context': server_context,
                            'multiple': multiple,
                        }
                        classes.append(new_test_class(NormalBinaryTest, vars))
                        classes.append(new_test_class(AcceleratedBinaryTest,
                            vars))
                        # header client to non-header server hangs
                        if server_header:
                            classes.append(new_test_class(HeaderTest, vars))

    if fastproto is not None:
        classes.append(new_test_class(HeaderAcceleratedCompactTest, {
            'server_type': "TCppServer",
            'ssl': False,
            'server_header': False,
            'server_context': False,
            'multiple': False}))

    for cls in classes:
        setattr(module, cls.__name__, cls)

add_test_classes(sys.modules[__name__])
