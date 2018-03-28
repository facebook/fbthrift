#!/usr/bin/env python3
import asyncio
import sys
import traceback
import types
import unittest

from testing.clients import TestingService
from testing.types import I32List, Color, easy
from thrift.py3 import TransportError, get_client


async def bad_client_connect() -> None:
    async with get_client(TestingService, port=1) as client:
        await client.complex_action('foo', 'bar', 9, 'baz')


class ClientTests(unittest.TestCase):
    def test_annotations(self) -> None:
        annotations = TestingService.annotations
        self.assertIsInstance(annotations, types.MappingProxyType)
        self.assertTrue(annotations.get('py3.pass_context'))
        self.assertFalse(annotations.get('NotAnAnnotation'))
        self.assertEqual(annotations['fun_times'], 'yes')
        with self.assertRaises(TypeError):
            # You can't set attributes on builtin/extension types
            TestingService.annotations = {}

    def test_client_keyword_arguments(self) -> None:
        # Create a broken client
        client = TestingService()
        # This should not raise an exception
        with self.assertRaises(asyncio.InvalidStateError):
            client.complex_action(first='foo', second='bar', third=9, fourth='baz')

        with self.assertRaises(asyncio.InvalidStateError):
            client.complex_action('foo', 'bar', 9, 'baz')

    def test_none_arguments(self) -> None:
        client = TestingService()
        with self.assertRaises(TypeError):
            # missing argument
            client.take_it_easy(9)  # type: ignore
        with self.assertRaises(TypeError):
            # Should be an easy type
            client.take_it_easy(9, None)  # type: ignore
        with self.assertRaises(TypeError):
            # Should not be None
            client.takes_a_list(None)  # type: ignore
        with self.assertRaises(TypeError):
            # Should be a bool
            client.invert(None)  # type: ignore
        with self.assertRaises(TypeError):
            # None is not a Color
            client.pick_a_color(None)  # type: ignore
        with self.assertRaises(TypeError):
            # None is not an int
            client.take_it_easy(None, easy())  # type: ignore

    def test_TransportError(self) -> None:
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
            # The traceback should be short since it raises inside
            # the rpc call not down inside the guts of thrift-py3
            self.assertEqual(len(traceback.extract_tb(sys.exc_info()[2])), 3)

    def test_set_persistent_header(self) -> None:
        """
        This was causing a nullptr dereference and thus a segfault
        """
        loop = asyncio.get_event_loop()

        async def test() -> None:
            async with get_client(TestingService, port=1, headers={'foo': 'bar'}):
                pass

        loop.run_until_complete(test())

    def test_rpc_container_autoboxing(self) -> None:
        client = TestingService()

        with self.assertRaises(asyncio.InvalidStateError):
            client.takes_a_list([1, 2, 3])

        with self.assertRaises(asyncio.InvalidStateError):
            client.takes_a_list(I32List([1, 2, 3]))

        loop = asyncio.get_event_loop()
        with self.assertRaises(TypeError):
            # This is safe because we do type checks before we touch
            # state checks
            loop.run_until_complete(
                client.takes_a_list([1, 'b', 'three']))  # type: ignore

    def test_rpc_non_container_types(self) -> None:
        client = TestingService()
        with self.assertRaises(TypeError):
            client.complex_action(b'foo', 'bar', 'nine', fourth='baz')  # type: ignore

    def test_rpc_enum_args(self) -> None:
        client = TestingService()
        loop = asyncio.get_event_loop()
        with self.assertRaises(TypeError):
            loop.run_until_complete(client.pick_a_color(0))  # type: ignore

        with self.assertRaises(asyncio.InvalidStateError):
            loop.run_until_complete(client.pick_a_color(Color.red))

    def test_rpc_int_sizes(self) -> None:
        one = 2 ** 7 - 1
        two = 2 ** 15 - 1
        three = 2 ** 31 - 1
        four = 2 ** 63 - 1
        client = TestingService()
        loop = asyncio.get_event_loop()
        with self.assertRaises(asyncio.InvalidStateError):
            # means we passed type checks
            loop.run_until_complete(client.int_sizes(one, two, three, four))

        with self.assertRaises(OverflowError):
            loop.run_until_complete(client.int_sizes(two, two, three, four))

        with self.assertRaises(OverflowError):
            loop.run_until_complete(client.int_sizes(one, three, three, four))

        with self.assertRaises(OverflowError):
            loop.run_until_complete(client.int_sizes(one, two, four, four))

        with self.assertRaises(OverflowError):
            loop.run_until_complete(client.int_sizes(one, two, three, four * 10))
