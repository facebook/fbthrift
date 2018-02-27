#!/usr/bin/env python3
import asyncio
import types
import unittest

from testing.services import TestingServiceInterface
from testing.types import easy, Color
from thrift.py3 import RequestContext, ThriftServer
from thrift.py3.server import SocketAddress
from typing import Sequence


class Handler(TestingServiceInterface):
    @TestingServiceInterface.pass_context_invert
    async def invert(self, ctx: RequestContext, value: bool) -> bool:
        return not value

    async def complex_action(
        self, first: str, second: str, third: int, fourth: str
    ) -> int:
        return third

    async def takes_a_list(self, ints: Sequence[int]) -> None:
        pass

    async def take_it_easy(self, how: int, what: easy) -> None:
        pass

    async def pick_a_color(self, color: Color) -> None:
        pass

    async def int_sizes(self, one: int, two: int, three: int, four: int) -> None:
        pass


class ServicesTests(unittest.TestCase):
    def test_get_address(self) -> None:
        loop = asyncio.get_event_loop()
        coro = self.get_address(loop)
        self.assertIsInstance(loop.run_until_complete(coro), SocketAddress)

    async def get_address(self, loop: asyncio.AbstractEventLoop) -> SocketAddress:
        server = ThriftServer(Handler(), port=0)
        serve_task = loop.create_task(server.serve())
        addy = await server.get_address()
        server.stop()
        await serve_task
        return addy

    def test_annotations(self) -> None:
        annotations = TestingServiceInterface.annotations
        self.assertIsInstance(annotations, types.MappingProxyType)
        self.assertTrue(annotations.get('py3.pass_context'))
        self.assertFalse(annotations.get('NotAnAnnotation'))
        self.assertEqual(annotations['fun_times'], 'yes')
        with self.assertRaises(TypeError):
            # You can't set attributes on builtin/extension types
            TestingServiceInterface.annotations = {}

    def test_ctx_unittest_call(self) -> None:
        # Create a broken client
        h = Handler()
        loop = asyncio.get_event_loop()
        coro = h.invert(RequestContext(), False)  # type: ignore
        value = loop.run_until_complete(coro)
        self.assertTrue(value)

    def test_unittest_call(self) -> None:
        h = Handler()
        loop = asyncio.get_event_loop()
        call = 5
        ret = loop.run_until_complete(h.complex_action('', '', call, ''))
        self.assertEqual(call, ret)
