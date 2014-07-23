from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
# this interferes with ServiceRouter/SWIG
# @lint-avoid-python-3-compatibility-imports
#from __future__ import unicode_literals

import multiprocessing
import sys
import threading
import time

from fb303.ContextFacebookBase import FacebookBase
from libfb.testutil import BaseFacebookTestCase
from thrift.transport import TSocket, TTransport
from thrift.protocol import TBinaryProtocol
from thrift.Thrift import TProcessorEventHandler
from thrift.server.TCppServer import TCppServer
from tools.test.stubs import fbpyunit

from test.sleep import SleepService, ttypes

TIMEOUT = 60 * 1000  # milliseconds

def getClient(port):
    transport = TSocket.TSocket('127.0.0.1', port)
    transport = TTransport.TFramedTransport(transport)
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = SleepService.Client(protocol)
    transport.open()
    return client


class SleepProcessorEventHandler(TProcessorEventHandler):
    def getHandlerContext(self, fn_name, server_context):
        self.last_peer_name = server_context.getPeerName()
        self.last_sock_name = server_context.getSockName()


class SleepHandler(FacebookBase, SleepService.Iface):
    def __init__(self):
        FacebookBase.__init__(self, "sleep")

    def sleep(self, seconds):
        print("server sleeping...")
        time.sleep(seconds)
        print("server sleeping... done")

    def space(self, str):
        return " ".join(str)


class SpaceProcess(multiprocessing.Process):
    def __init__(self, port):
        self.queue = multiprocessing.Queue()
        multiprocessing.Process.__init__(
            self, target=self.target, args=(self.queue,))
        self.port = port

    def target(self, queue):
        client = getClient(self.port)

        hw = "hello, world"
        hw_spaced = "h e l l o ,   w o r l d"

        assert client.space(hw) == hw_spaced

        queue.put((client._iprot.trans.getTransport().getSocketName(),
                   client._iprot.trans.getTransport().getPeerName()))


class ParallelProcess(multiprocessing.Process):
    def __init__(self, port):
        multiprocessing.Process.__init__(self)
        self.port = port

    def run(self):
        clients = []

        for i in xrange(0, 4):
            clients.append(getClient(self.port))

        for c in clients:
            c.send_sleep(3)

        for c in clients:
            c.recv_sleep()


class TestServer(BaseFacebookTestCase):
    def setUp(self):
        super(TestServer, self).setUp()
        processor = SleepService.Processor(SleepHandler())
        self.event_handler = SleepProcessorEventHandler()
        processor.setEventHandler(self.event_handler)
        self.server = TCppServer(processor)
        self.addCleanup(self.stopServer)
        # Let the kernel choose a port.
        self.server.setPort(0)
        self.server_thread = threading.Thread(target=self.server.serve)
        self.server_thread.start()

        for t in range(30):
            addr = self.server.getAddress()
            if addr:
                break
            time.sleep(0.1)

        self.assertTrue(addr)

        self.server_port = addr[1]

    def stopServer(self):
        if self.server:
            self.server.stop()
            self.server = None

    def testSpace(self):
        space = SpaceProcess(self.server_port)
        space.start()
        client_sockname, client_peername = space.queue.get()
        space.join()

        self.stopServer()

        self.assertEquals(space.exitcode, 0)
        self.assertEquals(self.event_handler.last_peer_name, client_sockname)
        self.assertEquals(self.event_handler.last_sock_name, client_peername)

    def testParallel(self):
        parallel = ParallelProcess(self.server_port)
        parallel.start()
        start_time = time.time()
        # this should take about 3 seconds.  In practice on an unloaded
        # box, it takes about 3.6 seconds.
        parallel.join()

        duration = time.time() - start_time
        print("total time = {}".format(duration))

        self.stopServer()

        self.assertEqual(parallel.exitcode, 0)
        self.assertLess(duration, 5)


if __name__ == '__main__':
    rc = fbpyunit.MainProgram(sys.argv).run()
    sys.exit(rc)
