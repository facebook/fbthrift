from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import unittest
import socket
from threading import Thread

from ThriftTest import ThriftTest

from thrift.transport import TSocket
from thrift.transport.THeaderTransport import MAX_BIG_FRAME_SIZE
from thrift.protocol import THeaderProtocol
from thrift.server import TCppServer


class TestHandler(ThriftTest.Iface):
    def testString(self, str):
        return str * 2**30


def create_server():
    processor = ThriftTest.Processor(TestHandler())
    server = TCppServer.TCppServer(processor)
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('0.0.0.0', 0))
    port = sock.getsockname()[1]
    server.setPort(port)

    t = Thread(name="test_tcpp_server", target=server.serve)
    t.setDaemon(True)
    t.start()

    return (server, port)


def create_client(port):
    socket = TSocket.TSocket("localhost", port)
    protocol = THeaderProtocol.THeaderProtocol(socket)
    protocol.trans.set_max_frame_size(MAX_BIG_FRAME_SIZE)
    protocol.trans.open()
    return ThriftTest.Client(protocol)


class BigFrameTest(unittest.TestCase):
    def testBigFrame(self):
        server, port = create_server()

        with create_client(port) as client:
            result = client.testString('a')
            self.assertEqual(len(result), 2**30)
