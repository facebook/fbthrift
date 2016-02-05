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
import math

from fb303.ContextFacebookBase import FacebookBase
from libfb.testutil import BaseFacebookTestCase
from thrift.transport import TSocket, TSSLSocket, TTransport, THeaderTransport
from thrift.protocol import TBinaryProtocol, THeaderProtocol
from thrift.Thrift import TProcessorEventHandler, TProcessor, TMessageType, \
    TServerInterface
from thrift.server.TCppServer import TCppServer, TSSLConfig, SSLPolicy, \
    SSLVerifyPeerEnum
from thrift.server.TServer import TServerEventHandler
from tools.test.stubs import fbpyunit

from concurrent.futures import ProcessPoolExecutor
from test.sleep import SleepService, ttypes
from futuretest.future import FutureSleepService

TIMEOUT = 60 * 1000  # milliseconds

def getClient(addr, sock_cls=TSocket.TSocket, service_module=SleepService):
    transport = TSocket.TSocket(addr[0], addr[1])
    transport = TTransport.TFramedTransport(transport)
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = service_module.Client(protocol)
    transport.open()
    return client

def getHeaderClient(addr, sock_cls=TSocket.TSocket):
    transport = sock_cls(addr[0], addr[1])
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
    def __init__(self, noop_event, shutdown_event):
        FacebookBase.__init__(self, "sleep")
        TServerInterface.__init__(self)
        self.noop_event = noop_event
        self.shutdown_event = shutdown_event

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

    def shutdown(self):
        self.shutdown_event.set()

    def header(self):
        request_context = self.getRequestContext()
        if request_context is None:
            return False
        headers = request_context.getHeaders()
        if headers is None:
            return False
        return headers.get(b"hello") == b"world"

def is_prime(num):
    if num % 2 == 0:
        return False

    sqrt_n = int(math.floor(math.sqrt(num)))
    for i in range(3, sqrt_n + 1, 2):
        if num % i == 0:
            return False

    return True

class FutureSleepHandler(FacebookBase, FutureSleepService.Iface,
        TServerInterface):
    def __init__(self, executor):
        FacebookBase.__init__(self, "futuresleep")
        TServerInterface.__init__(self)
        self.executor = executor

    def sleep(self, seconds):
        print("future server sleeping...")
        time.sleep(seconds)
        print("future server sleeping... done")

    def future_isPrime(self, num):
        return self.executor.submit(is_prime, num)

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

class FutureProcess(multiprocessing.Process):
    # Stolen from PEP-3148
    PRIMES = [
            112272535095293,
            112582705942171,
            112272535095293,
            115280095190773,
            115797848077099,
    ]

    def __init__(self, addr):
        multiprocessing.Process.__init__(self)
        self.addr = addr

    def isPrime(self, num):
        client = getClient(self.addr, FutureSleepService)
        assert client.isPrime(num)

    def sleep(self):
        client = getClient(self.addr, FutureSleepService)
        client.sleep(2)

    def run(self):
        threads = []
        # Send multiple requests from multiple threads. Unfortunately
        # future client isn't supported yet.
        for prime in FutureProcess.PRIMES:
            t = threading.Thread(target=self.isPrime, args=(prime,))
            t.start()
            threads.append(t)

        for i in range(5):
            t = threading.Thread(target=self.sleep)
            t.start()
            threads.append(t)

        for t in threads:
            t.join()

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
        client.shutdown()
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
        processor = SleepService.Processor(
                SleepHandler(self.noop_event, self.shutdown_event))
        self.event_handler = SleepProcessorEventHandler()
        processor.setEventHandler(self.event_handler)
        return processor

    def setUp(self):
        super(TestServer, self).setUp()
        self.noop_event = threading.Event()
        self.shutdown_event = threading.Event()
        self.serverEventHandler = TestServerEventHandler()
        self.server = TCppServer(self.getProcessor())
        self.configureSSL()
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

    def configureSSL(self):
        pass

    def stopServer(self):
        if self.server:
            self.server.stop()
            self.server = None

class FutureTestServer(TestServer):
    EXECUTOR = ProcessPoolExecutor()

    def getProcessor(self):
        return FutureSleepService.Processor(
                FutureSleepHandler(FutureTestServer.EXECUTOR))

    def testIsPrime(self):
        sleep = FutureProcess(self.server_addr)
        sleep.start()
        sleep.join()
        self.stopServer()

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
        self.assertTrue(self.shutdown_event.wait(5))

class HeaderTestServer(TestServer):
    def getProcessor(self):
        return TestHeaderProcessor()

    def testHeaderInProcessor(self):
        client = getHeaderClient(self.server_addr)
        self.assertEquals(client.space("hi"), "h i")
        self.stopServer()

class TSSLConfigTest(BaseFacebookTestCase):

    def testDefaults(self):
        config = TSSLConfig()
        self.assertEquals(config.cert_path, '')
        self.assertEquals(config.key_path, '')
        self.assertEquals(config.key_pw_path, '')
        self.assertEquals(config.client_ca_path, '')
        self.assertEquals(config.ecc_curve_name, '')
        self.assertEquals(config.verify, SSLVerifyPeerEnum.VERIFY)
        self.assertEquals(config.ssl_policy, SSLPolicy.PERMITTED)

    def testEnumSetters(self):
        config = TSSLConfig()
        bogus_values = ['', 'bogus', 5, 0]
        for v in bogus_values:
            with self.assertRaises(ValueError):
                config.verify = v

        for v in bogus_values:
            with self.assertRaises(ValueError):
                config.ssl_policy = v

class SSLHeaderTestServer(TestServer):
    def getProcessor(self):
        return TestHeaderProcessor()

    def configureSSL(self):
        config = TSSLConfig()
        self.assertEquals(config.key_path, "")
        config.ssl_policy = SSLPolicy.REQUIRED
        config.cert_path = 'thrift/test/py/test_cert.pem'
        config.client_verify = SSLVerifyPeerEnum.VERIFY
        config.key_path = None
        # expect an error with a cert_path but no key_path
        with self.assertRaises(ValueError):
            self.server.setSSLConfig(config)
        config.key_path = 'thrift/test/py/test_cert.pem'
        self.server.setSSLConfig(config)

    def testSSLClient(self):
        ssl_client = getHeaderClient(self.server_addr, TSSLSocket.TSSLSocket)
        self.assertEquals(ssl_client.space("hi"), "h i")
        client = getHeaderClient(self.server_addr)
        with self.assertRaises(Exception):
            client.space("hi")
        self.stopServer()


if __name__ == '__main__':
    rc = fbpyunit.MainProgram(sys.argv).run()
    sys.exit(rc)
