# @lint-avoid-pyflakes2
# @lint-avoid-python-3-compatibility-imports

import asyncio
import time

from thrift.server.TAsyncioServer import ThriftAsyncServerFactory
from thrift.util.asyncio import run_on_thread

from shared.ttypes import SharedStruct
from tutorial import Calculator
from tutorial.ttypes import Operation, InvalidOperation


class CalculatorHandler(Calculator.Iface):
    def __init__(self):
        self.log = {}

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

    @run_on_thread
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
        print('zip')


if __name__ == '__main__':
    # 1. Which loop we want to setup the server for?
    loop = asyncio.get_event_loop()
    handler = CalculatorHandler()
    # 2. Setup processors, event handlers and create the AsyncIO server.
    print('Creating the server...')
    server = loop.run_until_complete(
        ThriftAsyncServerFactory(handler, port=8848, loop=loop),
    )

    # Alternatively, if you'd like to asynchronously initialize multiple
    # servers at once, see the docstring of ThriftAsyncServerFactory.

    # 3. Explicitly runn the loop, which gives us flexibility to add more
    # servers, coroutines and other relevant setup.
    print('Running the server...')
    try:
        loop.run_forever()
    except KeyboardInterrupt:
        print("Server caught SIGINT, exiting.")
    finally:
        server.close()
        loop.close()
