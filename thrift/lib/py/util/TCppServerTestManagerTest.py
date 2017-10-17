from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import unittest
from thrift.Thrift import TApplicationException
from thrift.protocol import THeaderProtocol
from thrift.transport import TSocket
from thrift.transport import THeaderTransport
from thrift.util.TCppServerTestManager import TCppServerTestManager
from thrift.util.test_service import TestService
from thrift.util.test_service.ttypes import UserException2


class TestTCppServerTestManager(unittest.TestCase):

    class Handler(TestService.Iface):
        def __init__(self, data):
            self.__data = data

        def getDataById(self, id):
            return self.__data[id]

        def throwUserException(self):
            raise UserException2("Some message")

        def throwUncaughtException(self, msg):
            raise AssertionError(msg)

    def _perform_rpc(self, server, val):
        host, port = server.addr()
        with TSocket.TSocket(host=host, port=port) as sock:
            transport = THeaderTransport.THeaderTransport(sock)
            protocol = THeaderProtocol.THeaderProtocol(transport)
            client = TestService.Client(protocol, protocol)
            return client.getDataById(val)

    def test_with_handler(self):
        handler = self.Handler({7: "hello"})
        with TCppServerTestManager(handler) as server:
            data = self._perform_rpc(server, 7)
        self.assertEquals(data, "hello")

    def test_with_processor(self):
        handler = self.Handler({7: "hello"})
        processor = TestService.Processor(handler)
        with TCppServerTestManager(processor) as server:
            data = self._perform_rpc(server, 7)
        self.assertEquals(data, "hello")

    def test_with_server(self):
        handler = self.Handler({7: "hello"})
        processor = TestService.Processor(handler)
        server = TCppServerTestManager.make_server(processor)
        with TCppServerTestManager(server) as server:
            data = self._perform_rpc(server, 7)
        self.assertEquals(data, "hello")

    def test_throw_populates_headers(self):
        handler = self.Handler({7: "hello"})
        processor = TestService.Processor(handler)
        server = TCppServerTestManager.make_server(processor)
        with TCppServerTestManager(server) as server:
            host, port = server.addr()
            with TSocket.TSocket(host=host, port=port) as sock:
                transport = THeaderTransport.THeaderTransport(sock)
                protocol = THeaderProtocol.THeaderProtocol(transport)
                client = TestService.Client(protocol, protocol)

                try:
                    client.throwUserException()
                    self.fail('Expect to throw UserException2')
                except UserException2:
                    pass

                self.assertEquals("UserException2", transport.get_headers()["uex"])
                self.assertIn("Some message", transport.get_headers()["uexw"])

                try:
                    client.throwUncaughtException("a message!")
                    self.fail('Expect to throw TApplicationException')
                except TApplicationException:
                    pass

                self.assertEquals(
                    "TApplicationException", transport.get_headers()["uex"])
                self.assertIn(
                    "a message!", transport.get_headers()["uexw"])
