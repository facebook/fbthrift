from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

try:
    import asyncio
except:
    import trollius as asyncio

from thrift_asyncio.tutorial import Calculator as AsyncCalculator
from thrift_asyncio.sleep import Sleep as AsyncSleep
from tutorial import Calculator


class AsyncCalculatorHandler(AsyncCalculator.Iface):
    @asyncio.coroutine
    def add(self, n1, n2):
        return 42

    @asyncio.coroutine
    def calculate(self, logid, work):
        return 0

    @asyncio.coroutine
    def zip(self):
        print('zip')


class CalculatorHandler(Calculator.Iface):

    def add(self, n1, n2):
        return 42

    def calculate(self, logid, work):
        return 0

    def zip(self):
        print('zip')


class AsyncSleepHandler(AsyncSleep.Iface):

    def __init__(self, loop):
        self._loop = loop

    @asyncio.coroutine
    def echo(self, message, delay):
        return asyncio.sleep(delay, result=message, loop=self._loop)
