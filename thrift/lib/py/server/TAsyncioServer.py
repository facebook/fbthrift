# @lint-avoid-python-3-compatibility-imports

import asyncio
import logging
from io import BytesIO
from struct import unpack
from .TServer import TServer, TServerEventHandler, TConnectionContext
from thrift.transport.TTransport import TMemoryBuffer, TTransportException
from thrift.protocol.THeaderProtocol import THeaderProtocol, \
        THeaderProtocolFactory

class AsyncioRpcConnectionContext(TConnectionContext):
    def __init__(self, client_socket):
        self._client_socket = client_socket

    def getPeerName(self):
        return self._client_socket.getpeername()

class FramedProtocol(asyncio.Protocol):
    MAX_LENGTH = 1 << 24

    def __init__(self,):
        self.recvd = b""

    def data_received(self, data):
        self.recvd = self.recvd + data
        while len(self.recvd) >= 4:
            length, = unpack("!I", self.recvd[:4])
            if length > self.MAX_LENGTH:
                logging.error("Frame size is too large, is client/server"
                        " using HeaderTransport?")
                self.transport.close()
                return
            elif length == 0:
                logging.error("Empty frame")
                self.transport.close()
                return

            if len(self.recvd) < length + 4:
                return

            frame = self.recvd[0:4 + length]
            self.recvd = self.recvd[4 + length:]
            asyncio.async(self.message_received(frame))

    def message_received(self, frame):
        raise NotImplementedError

class SenderTransport():
    MAX_QUEUE_SIZE = 1 << 10

    def __init__(self, trans):
        self._queue = asyncio.Queue(maxsize=self.MAX_QUEUE_SIZE)
        self._trans = trans
        asyncio.async(self._send())

    @asyncio.coroutine
    def _send(self,):
        while True:
            msg = yield from self._queue.get()
            self._trans.write(msg)

    def send_message(self, msg):
        asyncio.async(self._queue.put(msg))

class WrappedTransport(TMemoryBuffer):
    def __init__(self, trans):
        TMemoryBuffer.__init__(self)
        self._trans = trans

    def flush(self):
        self._trans.send_message(self._writeBuffer.getvalue())
        self._writeBuffer = BytesIO()

class WrappedTransportFactory:
    def getTransport(self, trans):
        return WrappedTransport(trans)

class ThriftHeaderClientProtocol(FramedProtocol):
    def __init__(self, client_class):
        FramedProtocol.__init__(self)
        self._client_class = client_class

    def connection_made(self, transport):
        self.transport = transport
        self.client = self._client_class(
                SenderTransport(self.transport),
                WrappedTransportFactory(),
                THeaderProtocolFactory())

    def connection_lost(self, exc):
        for fut in self.client._futures.values():
            te = TTransportException(
                    type=TTransportException.END_OF_FILE,
                    message="Connection closed")
            if not fut.done():
                fut.set_exception(te)

    @asyncio.coroutine
    def message_received(self, frame):
        tmi = TMemoryBuffer(frame)
        iprot = THeaderProtocol(tmi)
        (fname, mtype, rseqid) = iprot.readMessageBegin()

        method = getattr(self.client, "recv_" + fname.decode(), None)
        if method is None:
            logging.error("Method " + fname + " isn't supported, bug?")
            self.transport.close()
        else:
            method(iprot, mtype, rseqid)

class ThriftClientProtocolFactory:
    def __init__(self, client_class):
        self.client_class = client_class

    def __call__(self,):
        return ThriftHeaderClientProtocol(self.client_class)

class ThriftHeaderServerProtocol(FramedProtocol):
    MAX_LENGTH = 1 << 24

    def __init__(self, processor, server_event_handler):
        self.recvd = b""
        self.processor = processor
        self.server_event_handler = server_event_handler
        self.server_context = None

    @asyncio.coroutine
    def message_received(self, frame):
        tmi = TMemoryBuffer(frame)
        tmo = TMemoryBuffer()

        iprot = THeaderProtocol(tmi)
        oprot = THeaderProtocol(tmo)

        try:
            yield from self.processor.process(iprot, oprot, self.server_context)
            msg = tmo.getvalue()
            if len(msg) > 0:
                self.transport.write(msg)
        except Exception:
            logging.exception("Exception while processing request")
            self.transport.close()

    def connection_made(self, transport):
        self.transport = transport
        socket = self.transport.get_extra_info("socket")
        if socket is not None:
            self.server_context = AsyncioRpcConnectionContext(socket)
        self.server_event_handler.newConnection(self.server_context)

    def connection_lost(self, exc):
        self.server_event_handler.connectionDestroyed(self.server_context)

class ThriftServerProtocolFactory:
    def __init__(self, processor, server_event_handler):
        self.processor = processor
        self.server_event_handler = server_event_handler

    def __call__(self,):
        return ThriftHeaderServerProtocol(self.processor,
                self.server_event_handler)

class TAsyncioServer(TServer):
    """Server built with asyncio.

    In the spirit of not giving too many choices that people don't
    understand well enough, this server only supports header protocol.

    Methods in a handler can be:
    1. undecorated. They will run in the event loop's executor threads.
       DO NOT use yield from here because the method is invoked by the
       executor (method with yield from becomes a coroutine function and
       invoking it returns a coroutine object without running it).
    2. decorated with @util.asyncio.run_in_loop. They will run in the event
       loop thread directly. Long running methods should NOT use this
       otherwise the event loop will be blocked. Also, DO NOT use yield from
       here for the same reason in 1.
    3. decorated with @asyncio.coroutine. They become coroutines and
       can implement any async processing using yield from. For example,
       call async services, schedule tasks with customized executors, etc.
    """
    def __init__(self, processor, port, nthreads=None, host=None):
        self.processor = self._getProcessor(processor)
        self.port = port
        self.host = host
        self.nthreads = nthreads
        self.server_event_handler = TServerEventHandler()

    def setServerEventHandler(self, handler):
        self.server_event_handler = handler

    def serve(self):
        self._start_server()

        try:
            self.loop.run_forever()
        except KeyboardInterrupt:
            logging.info("Server killed, exitting.")
        finally:
            self.server.close()
            self.loop.close()

    def _start_server(self):
        try:
            protocol_factory = ThriftServerProtocolFactory(self.processor,
                    self.server_event_handler)
            self.loop = asyncio.get_event_loop()
            if self.nthreads is not None:
                from concurrent.futures import ThreadPoolExecutor
                self.loop.set_default_executor(ThreadPoolExecutor(
                    max_workers=int(self.nthreads)))
            coro = self.loop.create_server(protocol_factory,
                    self.host, self.port)
            self.server = self.loop.run_until_complete(coro)
        except Exception as e:
            logging.error("Could not start server at port {}"
                    .format(self.port), e)

        if hasattr(self.server, "sockets"):
            for socket in self.server.sockets:
                self.server_event_handler.preServe(socket.getsockname())
