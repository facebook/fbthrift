from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import threading
import unittest

from thrift.Thrift import TApplicationException, TPriority
from thrift.protocol import THeaderProtocol
from thrift.transport import TSocket
from thrift.transport import THeaderTransport
from thrift.transport.TTransport import TTransportException
from thrift.util.TCppServerTestManager import TCppServerTestManager
from thrift.util.test_service import (
    TestService,
    PriorityService,
    SubPriorityService
)
from thrift.util.test_service.ttypes import UserException2


class BaseTest(unittest.TestCase):
    def _perform_rpc(self, server, service, method, *args, **kwargs):
        # Default 5s timeout
        return self._expiring_rpc(
            server, service, method, 5 * 1000, *args, **kwargs)

    # Same but with a timeout
    def _expiring_rpc(self, server, service, method, tm, *args, **kwargs):
        host, port = server.addr()
        with TSocket.TSocket(host=host, port=port) as sock:
            sock.setTimeout(tm)
            transport = THeaderTransport.THeaderTransport(sock)
            protocol = THeaderProtocol.THeaderProtocol(transport)
            client = service.Client(protocol, protocol)
            return getattr(client, method)(*args, **kwargs)


class TestTCppServerTestManager(BaseTest):
    class Handler(TestService.Iface):
        def __init__(self, data):
            self.__data = data

        def getDataById(self, id):
            return self.__data[id]

        def throwUserException(self):
            raise UserException2("Some message")

        def throwUncaughtException(self, msg):
            raise AssertionError(msg)

    def _perform_getDataById(self, server, val):
        return self._perform_rpc(server, TestService, 'getDataById', val)

    def test_with_handler(self):
        handler = self.Handler({7: "hello"})
        with TCppServerTestManager(handler) as server:
            data = self._perform_getDataById(server, 7)
        self.assertEquals(data, "hello")

    def test_with_processor(self):
        handler = self.Handler({7: "hello"})
        processor = TestService.Processor(handler)
        with TCppServerTestManager(processor) as server:
            data = self._perform_getDataById(server, 7)
        self.assertEquals(data, "hello")

    def test_with_server(self):
        handler = self.Handler({7: "hello"})
        processor = TestService.Processor(handler)
        server = TCppServerTestManager.make_server(processor)
        with TCppServerTestManager(server) as server:
            data = self._perform_getDataById(server, 7)
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


class TestTCppServerPriorities(BaseTest):
    class PriorityHandler(PriorityService.Iface):
        event = threading.Event()
        stuck = threading.Event()

        def bestEffort(self):
            return True

        def normal(self):
            return True

        def important(self):
            return True

        def unspecified(self):
            return True

    class SubPriorityHandler(PriorityService.Iface):
        def child_unspecified(self):
            return True

        def child_highImportant(self):
            return True

    def test_processor_priorities(self):
        handler = self.PriorityHandler()
        processor = PriorityService.Processor(handler)

        # Did we parse annotations correctly
        self.assertEquals(
            processor.get_priority('bestEffort'),
            TPriority.BEST_EFFORT
        )
        self.assertEquals(
            processor.get_priority('normal'),
            TPriority.NORMAL
        )
        self.assertEquals(
            processor.get_priority('important'),
            TPriority.IMPORTANT
        )
        self.assertEquals(
            processor.get_priority('unspecified'),
            TPriority.HIGH
        )

    def test_processor_child_priorities(self):
        handler = self.SubPriorityHandler()
        processor = SubPriorityService.Processor(handler)

        # Parent priorities present in extended services
        # Make sure parent service priorities don't leak to child services
        self.assertEquals(
            processor.get_priority('bestEffort'),
            TPriority.BEST_EFFORT
        )
        self.assertEquals(
            processor.get_priority('normal'),
            TPriority.NORMAL
        )
        self.assertEquals(
            processor.get_priority('important'),
            TPriority.IMPORTANT
        )
        self.assertEquals(
            processor.get_priority('unspecified'),
            TPriority.HIGH
        )

        # Child methods
        self.assertEquals(
            processor.get_priority('child_unspecified'),
            TPriority.NORMAL
        )
        self.assertEquals(
            processor.get_priority('child_highImportant'),
            TPriority.HIGH_IMPORTANT
        )

    def test_header_priorities(self):
        pass

    def test_server_queues(self):
        handler = self.PriorityHandler()
        processor = PriorityService.Processor(handler)

        # Make sure there are 0 threads for best_effort and 1 queue slot
        # (the queue size cannot be set to 0)
        cppserver = TCppServerTestManager.make_server(processor)
        cppserver.setNewPriorityThreadManager(
            best_effort=0,
            normal=1,
            important=1,
            high=0,
            high_important=0,
            enableTaskStats=False,
            maxQueueLen=1
        )

        # Since we'll have a Cpp2Worker stuck, don't wait for it to exit
        cppserver.setWorkersJoinTimeout(0)

        with TCppServerTestManager(cppserver) as server:
            # Send a request to the server and return immediately
            try:
                self._expiring_rpc(server, PriorityService, 'bestEffort', 0)
            except TTransportException:
                pass

            # The queue for bestEffort should be full, as the first request
            # will never get processed (best_effort=0)
            with self.assertRaises(TApplicationException):
                self._perform_rpc(server, PriorityService, 'bestEffort')

            # However the normal prio one should go through
            self.assertTrue(
                self._perform_rpc(server, PriorityService, 'normal'))

            cppserver.getThreadManager().clearPending()
