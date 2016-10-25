from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import trollius as asyncio

from trollius import (From, Return, )
from thrift.server.TTrolliusServer import ThriftClientProtocolFactory
from thrift.util.Decorators import protocol_manager


@asyncio.coroutine
def async_protocol_manager(coro):
    _, protocol = yield From(coro)
    raise Return(protocol_manager(protocol))


def create_client(client_klass,
                  host=None,
                  port=None,
                  loop=None,
                  timeouts=None,
                  client_type=None):
    """
    create a Trollius thrift client and return a context manager for it
    This is a coroutine
    :param client_klass: thrift Client class
    :param host: hostname/ip, None = loopback
    :param port: port number
    :param loop: Trollius event loop
    :returns: a Context manager which provides the thrift client
    """
    if not loop:
        loop = asyncio.get_event_loop()
    coro = loop.create_connection(
        ThriftClientProtocolFactory(
            client_klass,
            loop=loop,
            timeouts=timeouts,
            client_type=client_type,
        ),
        host=host,
        port=port,
    )
    return async_protocol_manager(coro)


def call_as_future(callable, loop, *args, **kwargs):
    """This is a copy of thrift.util.asyncio. So, let's consider unifying them.

        call_as_future(callable, *args, **kwargs) -> trollius.Task

    Like trollius.ensure_future() but takes any callable and converts
    it to a coroutine function first.
    """
    if not asyncio.iscoroutinefunction(callable):
        callable = asyncio.coroutine(callable)

    return asyncio.ensure_future(callable(*args, **kwargs), loop=loop)
