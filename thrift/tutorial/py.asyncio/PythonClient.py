# @lint-avoid-pyflakes2
# @lint-avoid-python-3-compatibility-imports

import asyncio
import time

from thrift.server.TAsyncioServer import ThriftClientProtocolFactory

from tutorial import Calculator
from tutorial.ttypes import Work, Operation, InvalidOperation


@asyncio.coroutine
def main(loop):
    (transport, protocol) = yield from loop.create_connection(
            ThriftClientProtocolFactory(Calculator.Client),
            host="127.0.0.1",
            port=8848)
    client = protocol.client

    # Wait for the server to solve this super hard problem indefinitely.
    sum = yield from asyncio.wait_for(client.add(1, 2), None)
    print("1 + 2 = {}".format(sum))

    # Try divide by zero.
    try:
        work = Work(num1=2, num2=0, op=Operation.DIVIDE)
        yield from asyncio.wait_for(client.calculate(1, work), None)
    except InvalidOperation as e:
        print("InvalidOperation: {}".format(e))

    # Make a few asynchronous calls concurrently and wait for all of them.
    start = time.time()
    calls = [
            client.add(2, 3),
            client.add(1990, 1991),
            client.calculate(2, Work(num1=4, num2=2, op=Operation.MULTIPLY)),
            client.calculate(3, Work(num1=9, num2=3, op=Operation.SUBTRACT)),
            client.calculate(4, Work(num1=6, num2=8, op=Operation.ADD)),
            client.ping(),
            client.zip(),
            ]
    done, pending = yield from asyncio.wait(calls)
    if len(done) != len(calls):
        raise RuntimeError("Not all calls finished!")
    time_spent = time.time() - start
    print("Time spent on processing {} requests: {:f} secs, results are:"
            .format(len(calls), time_spent))
    for fut in done:
        print(fut.result())
    transport.close()
    protocol.close()

if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    loop.run_until_complete(main(loop))
    loop.close()
