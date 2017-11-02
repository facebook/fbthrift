#!/usr/bin/env python3
import asyncio
import unittest

from testing.clients import TestingService
from testing.types import I32List, Color
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

    def test_rpc_container_autoboxing(self):
        client = TestingService()
        client.takes_a_list([1, 2, 3])
        client.takes_a_list(I32List([1, 2, 3]))
        loop = asyncio.get_event_loop()
        with self.assertRaises(TypeError):
            # This is safe because we do type checks before we touch
            # state checks
            loop.run_until_complete(client.takes_a_list([1, 'b', 'three']))

    def test_rpc_non_container_types(self):
        client = TestingService()
        with self.assertRaises(TypeError):
            client.complex_action(b'foo', 'bar', 'nine', fourth='baz')


    def test_rpc_enum_args(self):
        client = TestingService()
        loop = asyncio.get_event_loop()
        with self.assertRaises(ValueError):
            loop.run_until_complete(client.pick_a_color(0))

        with self.assertRaises(asyncio.InvalidStateError):
            loop.run_until_complete(client.pick_a_color(Color.red))
