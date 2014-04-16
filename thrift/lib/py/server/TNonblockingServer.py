#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#
"""Implementation of non-blocking server.

The main idea of the server is reciving and sending requests
only from main thread.

It also makes thread pool server in tasks terms, not connections.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import logging
import select
import socket
import struct
import sys
import threading
import time
import errno

if sys.version_info[0] >= 3:
    import queue
    Queue = queue
    xrange = range
else:
    import Queue

from thrift.server import TServer
from thrift.Thrift import TProcessor, TApplicationException
from thrift.transport import TTransport, TSocket
from thrift.protocol.TBinaryProtocol import TBinaryProtocolFactory
from thrift.protocol.THeaderProtocol import THeaderProtocolFactory

LOG_ERRORS_EVERY = 60 * 10    # once per 10 minutes

__all__ = ['TNonblockingServer']

class Worker(threading.Thread):
    """Worker is a small helper to process incoming connection."""
    def __init__(self, queue):
        threading.Thread.__init__(self)
        self.queue = queue

    def run(self):
        """Process queries from task queue, stop if processor is None."""
        while True:
            try:
                processor, iprot, oprot, otrans, callback = self.queue.get()
                if processor is None:
                    break
                callback.getContext().setProtocols(iprot, oprot)
                processor.process(iprot, oprot, callback.getContext())
                callback.success(reply=otrans.getvalue())
            except Exception:
                logging.exception("Exception while processing request")
                callback.failure()

WAIT_LEN = 0
WAIT_MESSAGE = 1
WAIT_PROCESS = 2
SEND_ANSWER = 3
CLOSED = 4

def locked(func):
    "Decorator which locks self.lock."
    def nested(self, *args, **kwargs):
        self.lock.acquire()
        try:
            return func(self, *args, **kwargs)
        finally:
            self.lock.release()
    return nested

def socket_exception(func):
    "Decorator close object on socket.error."
    def read(self, *args, **kwargs):
        try:
            return func(self, *args, **kwargs)
        except socket.error:
            self.close()
    return read

class Connection:
    """Basic class is represented connection.

    It can be in state:
        WAIT_LEN --- connection is reading request len.
        WAIT_MESSAGE --- connection is reading request.
        WAIT_PROCESS --- connection has just read whole request and
            waits for call ready routine.
        SEND_ANSWER --- connection is sending answer string (including length
            of answer).
        CLOSED --- socket was closed and connection should be deleted.
    """
    def __init__(self, client_socket, server):
        self.socket = client_socket.getHandle()
        self.socket.setblocking(False)
        self.lock = threading.RLock()
        self._server = server
        self._timer = None
        self.len = 0
        self.message = b''
        self.context = TServer.TRpcConnectionContext(client_socket)
        self._server.serverEventHandler.newConnection(self.context)
        self._set_status(WAIT_LEN)

    @locked
    def _set_status(self, status):
        self.status = status
        if self.status in (WAIT_LEN, WAIT_MESSAGE):
            self._server.poller.read(self.fileno())
            if self._server._readTimeout is not None:
                if self._timer is not None:
                    self._timer.cancel()
                self._timer = threading.Timer(self._server._readTimeout,
                        self._cleanup)
                self._timer.start()
        elif self._timer is not None:
            self._timer.cancel()

        if self.status == SEND_ANSWER:
            self._server.poller.write(self.fileno())

        if self.status in (WAIT_PROCESS, CLOSED):
            self._server.poller.unregister(self.fileno())


    def getContext(self):
        return self.context

    def success(self, reply):
        self.ready(True, reply)

    def failure(self):
        self.ready(False, b'')

    def _read_len(self):
        """Reads length of request."""

        read = self.socket.recv(4)
        if len(read) == 0:
            # if we read 0 bytes and self.message is empty, it means client
            # closed the connection
            if len(self.message) != 0:
                logging.error("can't read frame size from socket")
            self.close()
            return
        self.message += read
        if len(self.message) == 4:
            self.len, = struct.unpack(b'!i', self.message)
            if self.len < 0:
                logging.error("negative frame size, it seems client"\
                    " doesn't use FramedTransport")
                self.close()
            elif self.len == 0:
                logging.error("empty frame, it's really strange")
                self.close()
            else:
                self.len += 4   # Include message length
                self._set_status(WAIT_MESSAGE)

    @socket_exception
    def read(self):
        """Reads data from stream and switch state."""
        assert self.status in (WAIT_LEN, WAIT_MESSAGE)

        if self.status == WAIT_LEN:
            self._read_len()
            # go back to the main loop here for simplicity instead of
            # falling through, even though there is a good chance that
            # the message is already available
        elif self.status == WAIT_MESSAGE:
            read = self.socket.recv(self.len - len(self.message))
            if len(read) == 0:
                logging.error("can't read frame from socket" +
                              " (got %d of %d bytes)" %
                    (len(self.message), self.len))
                self.close()
                return
            self.message += read
            if len(self.message) == self.len:
                self._set_status(WAIT_PROCESS)

    @socket_exception
    def write(self):
        """Writes data from socket and switch state."""
        assert self.status == SEND_ANSWER
        sent = self.socket.send(self.message)
        if sent == len(self.message):
            self._set_status(WAIT_LEN)
            self.message = b''
            self.len = 0
        else:
            self.message = self.message[sent:]

    @locked
    def ready(self, all_ok, message):
        """Callback function for switching state and waking up main thread.

        This function is the only function which can be called asynchronous.

        The ready can switch Connection to three states:
            WAIT_LEN if request was oneway.
            SEND_ANSWER if request was processed in normal way.
            CLOSED if request throws unexpected exception.

        The one wakes up main thread.
        """
        assert self.status == WAIT_PROCESS
        if not all_ok:
            self.close()
            self._server.wake_up()
            return
        self.len = 0
        if len(message) == 0:
            # it was a oneway request, do not write answer
            self.message = b''
            self._set_status(WAIT_LEN)
        else:
            self.message = message
            self._set_status(SEND_ANSWER)
        self._server.wake_up()

    def fileno(self):
        "Returns the file descriptor of the associated socket."
        return self.socket.fileno()

    def close(self):
        "Closes connection"
        if self._timer is not None:
            self._timer.cancel()
        self._cleanup()

    def _cleanup(self):
        self._set_status(CLOSED)
        self._server.connection_closed(self)
        self.socket.close()

class TNonblockingServer(TServer.TServer):
    """Non-blocking server."""
    def __init__(self, processor, lsocket, inputProtocolFactory=None,
            outputProtocolFactory=None, threads=10, readTimeout=None,
            maxQueueSize=0):
        self.processor = self._getProcessor(processor)
        self.socket = lsocket
        self.in_protocol = inputProtocolFactory or TBinaryProtocolFactory()
        self.out_protocol = outputProtocolFactory or self.in_protocol
        self.threads = int(threads)
        self.clients = {}
        self.max_queue_size = maxQueueSize  # do not set this as a hard size
                                            # maximum - the queue may need
                                            # extra space for close()
        self.tasks = Queue.Queue()
        self._read, self._write = _create_socketpair()
        self.prepared = False
        self._stop = False
        self.serverEventHandler = TServer.TServerEventHandler()
        self.select_timeout = DEFAULT_SELECT_TIMEOUT
        self.poller = TSocket.ConnectionEpoll() if hasattr(select, "epoll") \
                else TSocket.ConnectionSelect()
        self.last_logged_error = 0
        timeouts = [x for x in [self.select_timeout, readTimeout] \
                        if x is not None]
        if len(timeouts) > 0:
            self.select_timeout = min(timeouts)
        self._readTimeout = readTimeout

    def setServerEventHandler(self, handler):
        """Sets the server event handler

        Arguments:
          handler: instance of TServer.TServerEventHandler
        """
        self.serverEventHandler = handler

    def setNumThreads(self, num):
        """Set the number of worker threads that should be created."""
        # implement ThreadPool interface
        assert not self.prepared, "You can't change number of threads for working server"
        self.threads = num

    def prepare(self):
        """Prepares server for serve requests."""
        if self.prepared:
            return
        self.socket.listen()
        for name in self.socket.getSocketNames():
            self.serverEventHandler.preServe(name)
        for _ in xrange(self.threads):
            thread = Worker(self.tasks)
            thread.setDaemon(True)
            thread.start()

        for fileno in self.socket.handles:
            self.poller.read(fileno)
        self.poller.read(self._read.fileno())

        self.prepared = True

    def wake_up(self):
        """Wake up main thread.

        The server usualy waits in select call in we should terminate one.
        The simplest way is using socketpair.

        Select always wait to read from the first socket of socketpair.

        In this case, we can just write anything to the second socket from
        socketpair."""
        self._write.send(b'1')

    def stop(self):
        """Stop the server.

        This method causes the serve() method to return.  stop() may be invoked
        from within your handler, or from another thread.

        After stop() is called, serve() will return but the server will still
        be listening on the socket.  serve() may then be called again to resume
        processing requests.  Alternatively, close() may be called after
        serve() returns to close the server socket and shutdown all worker
        threads.
        """
        self._stop = True
        self.wake_up()

    def log_poll_problem(self, msg):
        """ Logs errors every LOG_ERRORS_EVERY since they may occur
            frequently. """
        now = time.time()
        if now - self.last_logged_error >= LOG_ERRORS_EVERY:
            self.last_logged_error = now
            logging.exception(msg)

    def _select(self):
        """Does epoll or select on open connections."""
        try:
            return self.poller.process(self.select_timeout)
        except Exception as e:
            if not (isinstance(e, IOError) and e.errno == errno.EINTR):
                self.log_poll_problem("problem polling: %s" % e)
            return [], [], []

    def handle(self):
        """Handle requests.

        WARNING! You must call prepare BEFORE calling handle.
        """
        assert self.prepared, "You have to call prepare before handle"
        rset, wset, xset = self._select()
        for readable in rset:
            if readable == self._read.fileno():
                # don't care i just need to clean readable flag
                self._read.recv(1024)
            elif readable in self.socket.handles:
                client_socket = self.socket.accept()
                connection = Connection(client_socket, self)
                self.clients[client_socket.fileno()] = connection
            else:
                connection = self.clients[readable]
                connection.read()
                if connection.status == WAIT_PROCESS:
                    itransport = TTransport.TMemoryBuffer(connection.message)

                    # Header protocol needs oprot == iprot. This implies the
                    # input memory buffer is reused for output too.
                    if isinstance(self.in_protocol, THeaderProtocolFactory):
                        omembuf = itransport
                        iprot = self.in_protocol.getProtocol(itransport)
                        oprot = iprot
                    else:
                        # Otherwise, assume we need a TFramedTransport.
                        omembuf = TTransport.TMemoryBuffer()
                        itransport = TTransport.TFramedTransport(itransport)
                        otransport = TTransport.TFramedTransport(omembuf)
                        iprot = self.in_protocol.getProtocol(itransport)
                        oprot = self.out_protocol.getProtocol(otransport)

                    if self.max_queue_size == 0 or \
                            self.tasks.qsize() <= self.max_queue_size:
                        self.tasks.put([self.processor, iprot, oprot,
                                        omembuf, connection])
                    else:
                        logging.error(
                            "Queue max size of %d exceeded. Request rejected.",
                            self.max_queue_size)
        for writeable in wset:
            self.clients[writeable].write()
        for oob in xset:
            if oob in self.clients:
                connection = self.clients[oob]
                connection.close()

    def close(self):
        """Closes the server."""
        for _ in xrange(self.threads):
            self.tasks.put([None, None, None, None, None])
        self.socket.close()
        self.prepared = False

    def connection_closed(self, connection):
        """Connection close callback"""
        self.serverEventHandler.connectionDestroyed(connection.context)
        del self.clients[connection.fileno()]

    def serve(self):
        """Serve requests.

        Serve requests forever, or until stop() is called.
        """
        self._stop = False
        self.prepare()
        while not self._stop:
            self.handle()


if sys.platform == 'win32':
    # The Windows select() implementation can't process signals during select.
    # Default to a 1 second select timeout, so we'll wake up once a second to
    # process any pending signals.
    DEFAULT_SELECT_TIMEOUT = 1

    # socket.socketpair() isn't supported on windows, so define our own
    # equivalent
    def _create_socketpair():
        # Create a socket to listen on an ephemeral port
        listener = socket.socket()
        listener.bind(('127.0.0.1', 0))
        listen_addr = listener.getsockname()
        listener.listen(1)

        # Create a second socket to connect to it
        connector = socket.socket()
        # From my limited testing, just using a blocking connect seems to work.
        # If we wanted to be more careful, we could set connector to
        # non-blocking mode to perform the connect, so we can call accept on
        # the listener.
        connector.connect(listen_addr)

        (accepted, address) = listener.accept()

        listener.close()

        return (connector, accepted)
else:
    DEFAULT_SELECT_TIMEOUT = None
    _create_socketpair = socket.socketpair
