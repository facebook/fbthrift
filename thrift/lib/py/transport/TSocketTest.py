from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import socket

import thrift.transport.TSocket as TSocket

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
