from apache.thrift.test.asyncio.load import LoadTest
from apache.thrift.test.asyncio.load.ttypes import LoadError
import asyncio
from concurrent.futures import ProcessPoolExecutor
import pickle
import time


def us_to_sec(microseconds):
    return microseconds / 1000000


def burn_in_executor(us):
    '''
    The "busy wait" functions are CPU bound; to prevent blocking
    they need to be run in an executor. This function
    provides the implementation.
    '''
    start = time.time()
    end = start + us_to_sec(us)
    while time.time() < end:
        pass


class LoadHandler(LoadTest.Iface):
    def __init__(self, loop=None):
        super().__init__()
        self.loop = loop or asyncio.get_event_loop()
        self.pool = ProcessPoolExecutor()
        pickle.DEFAULT_PROTOCOL = pickle.HIGHEST_PROTOCOL

    def noop(self):
        pass

    def onewayNoop(self):
        pass

    def asyncNoop(self):
        pass

    @asyncio.coroutine
    def sleep(self, us):
        yield from asyncio.sleep(us_to_sec(us))

    @asyncio.coroutine
    def onewaySleep(self, us):
        yield from asyncio.sleep(us_to_sec(us))

    @asyncio.coroutine
    def burn(self, us):
        return (yield from self.loop.run_in_executor(self.pool,
            burn_in_executor, us))

    @asyncio.coroutine
    def onewayBurn(self, us):
        return (yield from self.loop.run_in_executor(self.pool,
            burn_in_executor, us))

    def badSleep(self, us):
        # "bad" because it sleeps on the main thread
        # tests how the infrastructure responds to poorly
        # written client requests
        time.sleep(us_to_sec(us))

    def badBurn(self, us):
        return burn_in_executor(us)

    def throwError(self, code):
        raise LoadError(code=code)

    def throwUnexpected(self, code):
        raise LoadError(code=code)

    def send(self, data):
        pass

    def onewaySend(self, data):
        pass

    def recv(self, bytes):
        return 'a' * bytes

    def sendrecv(self, data, recvBytes):
        return 'a' * recvBytes

    def echo(self, data):
        return data

    def add(self, a, b):
        return a + b

    def largeContainer(self, data):
        pass

    async def iterAllFields(self, data):
        for item in data:
            x = item.stringField
            for x in item.stringList:
                pass
        return data
