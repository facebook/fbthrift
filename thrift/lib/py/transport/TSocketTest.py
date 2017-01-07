from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import socket

import thrift.transport.TSocket as TSocket
import thrift.transport.TTransport as TTransport

import threading
import time
import unittest


class TSocketTest(unittest.TestCase):

    def test_usage_as_context_manager(self):
        """
        Asserts that both TSocket and TServerSocket can be used with `with` and
        that their resources are disposed of at the close of the `with`.
        """
        text = "hi"  # sample text to send over the wire
        with TSocket.TServerSocket(port=0, family=socket.AF_INET6) as server:
            addr = server.getSocketNames()[0]
            with TSocket.TSocket(host=addr[0], port=addr[1]) as conn:
                conn.write(text)
            self.assertFalse(conn.isOpen())
            with server.accept() as client:
                read = client.read(len(text))
            self.assertFalse(conn.isOpen())
        self.assertFalse(server.isListening())
        self.assertEquals(read, text)

    def test_server_context_errors(self):
        # Make sure the TServerSocket context manager doesn't
        # swallow exceptions
        def do_test():
            with TSocket.TServerSocket(port=0, family=socket.AF_INET6):
                raise Exception('test_error')

        self.assertRaisesRegexp(Exception, 'test_error', do_test)

    def test_open_failure(self):
        # Bind a server socket to an address, but don't actually listen on it.
        server_socket = socket.socket(socket.AF_INET6)
        try:
            server_socket.bind(('::', 0))
            server_port = server_socket.getsockname()[1]

            # Explicitly use "localhost" as the hostname, so that the
            # connect code will try both IPv6 and IPv4.  We want to
            # exercise the failure behavior when trying multiple addresses.
            sock = TSocket.TSocket(host='localhost', port=server_port)
            sock.setTimeout(50)  # ms
            try:
                sock.open()
                self.fail('unexpectedly succeeded to connect to closed socket')
            except TTransport.TTransportException:
                # sock.open() should not leave the file descriptor open
                # when it fails
                self.assertEquals(None, sock.handle)
                self.assertEquals({}, sock.handles)

                # Calling close() again on the socket should be a no-op,
                # and shouldn't throw an error
                sock.close()
        finally:
            server_socket.close()

    def test_poller_process(self):
        # Make sure that pollers do not fail when they're given None as timeout
        text = "hi"  # sample text to send over the wire
        with TSocket.TServerSocket(port=0, family=socket.AF_INET6) as server:
            addr = server.getSocketNames()[0]

            def write_data():
                # delay writing to verify that poller.process is waiting
                time.sleep(1)
                with TSocket.TSocket(host=addr[0], port=addr[1]) as conn:
                    conn.write(text)

            poller = TSocket.ConnectionSelect()
            thread = threading.Thread(target=write_data)
            thread.start()
            for filenos in server.handles.keys():
                poller.read(filenos)

            r, _, x = poller.process(timeout=None)

            thread.join()
            # Verify that r is non-empty
            self.assertTrue(r)
