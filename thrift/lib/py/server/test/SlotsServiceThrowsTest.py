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
from thrift.server.test.slots_throwing_service import SlotsThrowingService
from thrift.server.test.slots_throwing_service.ttypes import UserException2


class SlotsServiceThrowsTest(unittest.TestCase):

    class Handler(SlotsThrowingService.Iface):
        def throwUserException(self):
            raise UserException2("Some message")

        def throwUncaughtException(self, msg):
            raise AssertionError(msg)

    def _perform_rpc(self, server, val):
        host, port = server.addr()
        with TSocket.TSocket(host=host, port=port) as sock:
            transport = THeaderTransport.THeaderTransport(sock)
            protocol = THeaderProtocol.THeaderProtocol(transport)
            client = SlotsThrowingService.Client(protocol, protocol)
            return client.getDataById(val)

    def test_throw_populates_headers(self):
        handler = self.Handler()
        processor = SlotsThrowingService.Processor(handler)
        server = TCppServerTestManager.make_server(processor)
        with TCppServerTestManager(server) as server:
            host, port = server.addr()
            with TSocket.TSocket(host=host, port=port) as sock:
                transport = THeaderTransport.THeaderTransport(sock)
                protocol = THeaderProtocol.THeaderProtocol(transport)
                client = SlotsThrowingService.Client(protocol, protocol)

                try:
                    client.throwUserException()
                    self.fail('Expect to throw UserException2')
                except UserException2:
                    pass

                self.assertEquals(b"UserException2", transport.get_headers()[b"uex"])
                self.assertIn(b"Some message", transport.get_headers()[b"uexw"])

                try:
                    client.throwUncaughtException("a message!")
                    self.fail('Expect to throw TApplicationException')
                except TApplicationException:
                    pass

                self.assertEquals(
                    b"TApplicationException", transport.get_headers()[b"uex"])
                self.assertIn(
                    b"a message!", transport.get_headers()[b"uexw"])
