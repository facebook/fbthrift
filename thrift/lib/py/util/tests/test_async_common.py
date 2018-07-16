#!/usr/bin/env python3
import unittest
import mock

from thrift.util.async_common import  AsyncioRpcConnectionContext


class TestAsyncioRpcConnectionContext(unittest.TestCase):

    def test_getSockName(self):
        test_sock_laddr = (
            '198.51.100.29',  # TEST-NET-2
            2929,
            0,
            0
        )
        client_socket = mock.Mock()
        client_socket.getsockname.return_value = test_sock_laddr
        context = AsyncioRpcConnectionContext(client_socket)

        ret = context.getSockName()

        self.assertEquals(ret, test_sock_laddr)
