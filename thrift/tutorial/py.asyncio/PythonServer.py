# @lint-avoid-python-3-compatibility-imports

import time

from tutorial import Calculator
from tutorial.ttypes import *

from shared.ttypes import SharedStruct

from thrift.server import TAsyncioServer
from thrift.util.asyncio import run_in_loop
import asyncio

class CalculatorHandler(Calculator.Iface):
    def __init__(self):
        self.log = {}

    @run_in_loop
    def ping(self):
        """This method runs in the server's event loop directly."""
        print('ping()')

    @asyncio.coroutine
    def add(self, n1, n2):
        """This method is a coroutine and the server will yield from it."""
        print('add(%d,%d)' % (n1, n2))
        # Calling some other services asynchronously, it takes 1 second.
        yield from asyncio.sleep(1)
        return n1 + n2

    def calculate(self, logid, work):
        """By default, this method runs in the server's executor threads."""
        print('calculate(%d, %r)' % (logid, work))

        # Calculate the 10000000th prime number first, it takes 1 second.
        end = time.time() + 1
        while True:
            now = time.time()
            if now >= end:
                break

        if work.op == Operation.ADD:
            val = work.num1 + work.num2
        elif work.op == Operation.SUBTRACT:
            val = work.num1 - work.num2
        elif work.op == Operation.MULTIPLY:
            val = work.num1 * work.num2
        elif work.op == Operation.DIVIDE:
            if work.num2 == 0:
                x = InvalidOperation()
                x.what = work.op
                x.why = 'Cannot divide by 0'
                raise x
            val = work.num1 // work.num2
        else:
            x = InvalidOperation()
            x.what = work.op
            x.why = 'Invalid operation'
            raise x

        log = SharedStruct()
        log.key = logid
        log.value = '%d' % (val)
        self.log[logid] = log

        return val

    def getStruct(self, key):
        print('getStruct(%d)' % (key))
        return self.log[key]

    def zip(self):
        print('zip()')

handler = CalculatorHandler()
server = TAsyncioServer.TAsyncioServer(handler, 8848)

print('Starting the server...')
server.serve()
