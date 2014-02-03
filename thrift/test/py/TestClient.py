#!/usr/bin/env python

#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import sys, glob, os

sys.path.insert(0, './gen-py')
lib_path = glob.glob('../../lib/py/build/lib.*')
if lib_path:
    sys.path.insert(0, lib_path[0])

from ThriftTest import ThriftTest
from ThriftTest.ttypes import *
from thrift.transport import TTransport
from thrift.transport import TSocket
from thrift.transport import THttpClient
from thrift.transport import THeaderTransport
from thrift.protocol import TBinaryProtocol
from thrift.protocol import THeaderProtocol
import unittest
import time
from optparse import OptionParser


parser = OptionParser()
parser.set_defaults(framed=False, header=False, http_path=None, verbose=1,
                    host='localhost', port=9090)
parser.add_option("--port", type="int", dest="port",
    help="connect to server at port")
parser.add_option("--host", type="string", dest="host",
    help="connect to server")
parser.add_option("--framed", action="store_true", dest="framed",
    help="use framed transport")
parser.add_option("--header", action="store_true", dest="header",
    help="use header transport")
parser.add_option("--http", dest="http_path",
    help="Use the HTTP transport with the specified path")
parser.add_option('-v', '--verbose', action="store_const",
    dest="verbose", const=2,
    help="verbose output")
parser.add_option('-q', '--quiet', action="store_const",
    dest="verbose", const=0,
    help="minimal output")

options, args = parser.parse_args()

class AbstractTest():
    def setUp(self):
        if options.http_path:
            self.transport = THttpClient.THttpClient(
                options.host, options.port, options.http_path)
        else:
            socket = TSocket.TSocket(options.host, options.port)

            # frame or buffer depending upon args
            if options.framed:
                self.transport = TTransport.TFramedTransport(socket)
            else:
                self.transport = TTransport.TBufferedTransport(socket)

        protocol = self.protocol_factory.getProtocol(self.transport)
        if options.header:
            protocol = THeaderProtocol.THeaderProtocol(socket)
            self.transport = protocol.trans
            protocol.trans.add_transform(protocol.trans.ZLIB_TRANSFORM)

        self.transport.open()
        self.client = ThriftTest.Client(protocol)

    def tearDown(self):
        # Close!
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
        self.assertEquals(1, count)

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
            if isinstance(self.transport, THttpClient.THttpClient):
                # server event handlers not implemented for HTTP server;
                # these counters will always be 1 (priming in TestServer)
                self.assertEquals(orig_num_pre_serve, 1)
                self.assertEquals(orig_num_new, 1)
                self.assertEquals(orig_num_dest, 1)
                continue

            # force new server connection
            self.transport.getTransport().close()
            self.transport.getTransport().open()

            num_pre_serve = self.client.testPreServe()
            num_new = self.client.testNewConnection()
            num_dest = self.client.testConnectionDestroyed()

            # always exactly one pre-serve event
            self.assertEquals(num_pre_serve, 1)

            # every iteration is a new re-connection
            self.assertEquals(num_new, orig_num_new + i + 1)

            # every iteration is a new connection destroyed, and current
            # conneciton is active, so num_dest should be num_new - 1.
            # Except:
            # 1) TForkingServer does not update num_dest (or rather, the
            #    num_dest is updated in the child process, which doesn't
            #    make it back to the parent process), so num_dest is always
            #    0 in this case
            # 2) TNonblockingServer fires the connectionDestroyed() event
            #    on the top of the next handle() iteration, so the count
            #    could be off by 2 in this case rather than 1.
            if num_dest:
                self.assert_(num_new - num_dest <= 2)


class NormalBinaryTest(AbstractTest, unittest.TestCase):
    protocol_factory = TBinaryProtocol.TBinaryProtocolFactory()


class AcceleratedBinaryTest(AbstractTest, unittest.TestCase):
    protocol_factory = TBinaryProtocol.TBinaryProtocolAcceleratedFactory()


def suite():
    suite = unittest.TestSuite()
    loader = unittest.TestLoader()

    suite.addTest(loader.loadTestsFromTestCase(NormalBinaryTest))
    suite.addTest(loader.loadTestsFromTestCase(AcceleratedBinaryTest))
    return suite


class OwnArgsTestProgram(unittest.TestProgram):
    def parseArgs(self, argv):
        if args:
            self.testNames = args
        else:
            self.testNames = (self.defaultTest,)
        self.createTests()

if __name__ == "__main__":
    OwnArgsTestProgram(defaultTest="suite",
                       testRunner=unittest.TextTestRunner(verbosity=2))
