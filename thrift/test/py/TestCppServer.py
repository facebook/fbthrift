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
from thrift.transport import TSocket, TTransport, THeaderTransport
from thrift.protocol import TBinaryProtocol, THeaderProtocol
from thrift.Thrift import TProcessorEventHandler, TProcessor, TMessageType, \
        TServerInterface
from thrift.server.TCppServer import TCppServer
from thrift.server.TServer import TServerEventHandler
from tools.test.stubs import fbpyunit

from test.sleep import SleepService, ttypes

TIMEOUT = 60 * 1000  # milliseconds

def getClient(addr):
    transport = TSocket.TSocket(addr[0], addr[1])
    transport = TTransport.TFramedTransport(transport)
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = SleepService.Client(protocol)
    transport.open()
    return client

def getHeaderClient(addr):
    transport = TSocket.TSocket(addr[0], addr[1])
    transport = THeaderTransport.THeaderTransport(transport)
    transport.set_header("hello", "world")
    protocol = THeaderProtocol.THeaderProtocol(transport)
    client = SleepService.Client(protocol)
    transport.open()
    return client

class SleepProcessorEventHandler(TProcessorEventHandler):
    def getHandlerContext(self, fn_name, server_context):
        self.last_peer_name = server_context.getPeerName()
        self.last_sock_name = server_context.getSockName()

class SleepHandler(FacebookBase, SleepService.Iface, TServerInterface):
    def __init__(self, noop_event):
        FacebookBase.__init__(self, "sleep")
        TServerInterface.__init__(self)
        self.noop_event = noop_event

    def sleep(self, seconds):
        print("server sleeping...")
        time.sleep(seconds)
        print("server sleeping... done")

    def space(self, s):
        if isinstance(s, bytes):
            s = s.decode('latin1')

        return " ".join(s)

    def noop(self):
        self.noop_event.set()

    def header(self):
        request_context = self.getRequestContext()
        if request_context is None:
            return False
        headers = request_context.getHeaders()
        if headers is None:
            return False
        return headers.get(b"hello") == b"world"

class SpaceProcess(multiprocessing.Process):
    def __init__(self, addr):
        self.queue = multiprocessing.Queue()
        multiprocessing.Process.__init__(
            self, target=self.target, args=(self.queue,))
        self.addr = addr

    def target(self, queue):
        client = getClient(self.addr)

        hw = "hello, world"
        hw_spaced = "h e l l o ,   w o r l d"

        result = client.space(hw)
        if isinstance(result, bytes):
            result = result.decode('latin1')
        assert result == hw_spaced

        queue.put((client._iprot.trans.getTransport().getSocketName(),
                   client._iprot.trans.getTransport().getPeerName()))

class HeaderProcess(multiprocessing.Process):
    def __init__(self, addr):
        multiprocessing.Process.__init__(self)
        self.addr = addr

    def run(self):
        client = getHeaderClient(self.addr)
        assert client.header()

class ParallelProcess(multiprocessing.Process):
    def __init__(self, addr):
        multiprocessing.Process.__init__(self)
        self.addr = addr

    def run(self):
        clients = []

        for i in range(0, 4):
            clients.append(getClient(self.addr))

        for c in clients:
            c.send_sleep(3)

        for c in clients:
            c.recv_sleep()

class OnewayProcess(multiprocessing.Process):
    def __init__(self, addr):
        multiprocessing.Process.__init__(self)
        self.addr = addr

    def run(self):
        client = getClient(self.addr)

        hw = "hello, world"
        hw_spaced = "h e l l o ,   w o r l d"

        client.noop()
        # Requests sent after the oneway request still get responses
        result = client.space(hw)
        if isinstance(result, bytes):
            result = result.decode('latin1')
        assert result == hw_spaced

class TestHeaderProcessor(TProcessor, BaseFacebookTestCase):
    def process(self, iprot, oprot, server_context=None):
        fname, type, seqid = iprot.readMessageBegin()
        self.assertTrue(iprot.trans.get_headers().get(b"hello") == b"world")
        result = SleepService.space_result()
        result.success = "h i"
        oprot.writeMessageBegin(fname, TMessageType.REPLY, seqid)
        result.write(oprot)
        oprot.writeMessageEnd()
        oprot.trans.flush()

class TestServerEventHandler(TServerEventHandler):
    def __init__(self):
        self.connCreated = 0
        self.connDestroyed = 0

    def newConnection(self, context):
        self.connCreated += 1

    def connectionDestroyed(self, context):
        self.connDestroyed += 1

class TestServer(BaseFacebookTestCase):
    def getProcessor(self):
        processor = SleepService.Processor(SleepHandler(self.noop_event))
        self.event_handler = SleepProcessorEventHandler()
        processor.setEventHandler(self.event_handler)
        return processor

    def setUp(self):
        super(TestServer, self).setUp()
        self.noop_event = threading.Event()
        self.serverEventHandler = TestServerEventHandler()
        self.server = TCppServer(self.getProcessor())
        self.server.setServerEventHandler(self.serverEventHandler)
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

        self.server_addr = addr

    def stopServer(self):
        if self.server:
            self.server.stop()
            self.server = None

class BaseTestServer(TestServer):
    def testSpace(self):
        space = SpaceProcess(self.server_addr)
        space.start()
        client_sockname, client_peername = space.queue.get()
        space.join()

        self.stopServer()

        self.assertEquals(space.exitcode, 0)
        self.assertEquals(self.event_handler.last_peer_name, client_sockname)
        self.assertEquals(self.event_handler.last_sock_name, client_peername)
        self.assertEquals(self.serverEventHandler.connCreated, 1)
        self.assertEquals(self.serverEventHandler.connDestroyed, 1)

    def testHeader(self):
        header = HeaderProcess(self.server_addr)
        header.start()
        header.join()
        self.stopServer()

    def testParallel(self):
        parallel = ParallelProcess(self.server_addr)
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

    def testOneway(self):
        oneway = OnewayProcess(self.server_addr)
        oneway.start()
        oneway.join()

        self.stopServer()

        self.assertTrue(self.noop_event.wait(5))

class HeaderTestServer(TestServer):
    def getProcessor(self):
        return TestHeaderProcessor()

    def testHeaderInProcessor(self):
        client = getHeaderClient(self.server_addr)
        self.assertEquals(client.space("hi"), "h i")
        self.stopServer()

if __name__ == '__main__':
    rc = fbpyunit.MainProgram(sys.argv).run()
    sys.exit(rc)
