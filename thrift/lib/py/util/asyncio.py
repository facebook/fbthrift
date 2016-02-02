# @lint-avoid-pyflakes2
# @lint-avoid-python-3-compatibility-imports

import asyncio

from thrift.server import TAsyncioServer
from thrift.util.Decorators import (
    get_function_name,
    make_unknown_function_exception,
    protocol_manager,
)

def process_main(func):
    """Decorator for process method."""

    def nested(self, iprot, oprot, server_ctx=None):
        # self is a TProcessor instance
        name, seqid = self.readMessageBegin(iprot)
        if self.doesKnowFunction(name):
            yield from self._processMap[name](
                self, seqid, iprot, oprot, server_ctx,
            )
        else:
            self.skipMessageStruct(iprot)
            exc = make_unknown_function_exception(name)
            self.writeException(oprot, name, seqid, exc)
    return nested


def process_method(argtype, oneway=False):
    """Decorator for process_xxx methods for asyncio."""
    def _decorator(func):
        def nested(self, seqid, iprot, oprot, server_ctx):
            fn_name = get_function_name(func)
            # self is a TProcessor instance
            handler_ctx = self._event_handler.getHandlerContext(
                fn_name,
                server_ctx,
            )
            args = self.readArgs(iprot, handler_ctx, fn_name, argtype)

            result = yield from func(self, args, handler_ctx)
            if not oneway:
                self.writeReply(oprot, handler_ctx, fn_name, seqid, result)
        return nested
    return _decorator


def run_on_thread(func):
    func._run_on_thread = True
    return func


def should_run_on_thread(func):
    return getattr(func, "_run_on_thread", False)


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
