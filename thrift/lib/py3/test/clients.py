#!/usr/bin/env python3
import asyncio
import unittest

from testing.clients import TestingService
from thrift.py3 import TransportError, get_client


async def bad_client_connect():
    async with get_client(TestingService, port=1) as client:
        await client.complex_action('foo', 'bar', 9, 'baz')


class ClientTests(unittest.TestCase):
    def test_client_keyword_arguments(self):
        # Create a broken client
        client = TestingService()
        # This should not raise an exception
        client.complex_action(first='foo', second='bar', third=9, fourth='baz')
        client.complex_action('foo', 'bar', 9, 'baz')

    def test_TransportError(self):
        """
        Are C++ TTransportException converting properly to py TransportError
        """
        loop = asyncio.get_event_loop()
        with self.assertRaises(TransportError):
            loop.run_until_complete(bad_client_connect())

        try:
            loop.run_until_complete(bad_client_connect())
        except TransportError as ex:
            # Test that we can get the errno
            self.assertEqual(ex.errno, 0)

    def test_set_persistent_header(self):
        """
        This was causing a nullptr dereference and thus a segfault
        """
        loop = asyncio.get_event_loop()

        async def test():
            async with get_client(TestingService, port=1, headers={'foo': 'bar'}):
                pass

        loop.run_until_complete(test())
