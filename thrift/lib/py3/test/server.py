#!/usr/bin/env python3
import asyncio
import unittest

from testing.services import TestingServiceInterface
from testing.types import easy, Color
from thrift.py3 import RequestContext
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
