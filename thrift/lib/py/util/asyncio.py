# @lint-avoid-pyflakes2
# @lint-avoid-python-3-compatibility-imports

import asyncio

from thrift.server import TAsyncioServer
from thrift.util.Decorators import protocol_manager

@asyncio.coroutine
def create_client(client_klass, *, host=None, port=None, loop=None):
    """
    create a asyncio thrift client and return a context manager for it
    This is a coroutine
    :param client_klass: thrift Client class
    :param host: hostname/ip, None = loopback
    :param port: port number
    :param loop: asyncio event loop
    :returns: a Context manager which provides the thrift client
    """
    if not loop:
        loop = asyncio.get_event_loop()
    transport, protocol = yield from loop.create_connection(
        TAsyncioServer.ThriftClientProtocolFactory(client_klass),
        host=host, port=port)
    return protocol_manager(protocol)


def call_as_future(f, *args, **kwargs):
    """call_as_future(callable, *args, **kwargs) -> asyncio.Task

    Like asyncio.ensure_future() but takes any callable and converts
    it to a coroutine function first.
    """
    if not asyncio.iscoroutinefunction(f):
        f = asyncio.coroutine(f)

    return asyncio.ensure_future(f(*args, **kwargs))
