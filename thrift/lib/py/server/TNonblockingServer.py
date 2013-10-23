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
import Queue
import select
import socket
import struct
import sys
import threading
import time
import errno

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
    def __init__(self, client_socket, wake_up, server_event_handler):
        self.socket = client_socket.getHandle()
        self.socket.setblocking(False)
        self._set_status(WAIT_LEN)
        self.len = 0
        self.message = b''
        self.lock = threading.Lock()
        self.wake_up = wake_up
        self.server_event_handler = server_event_handler
        self.context = TServer.TRpcConnectionContext(client_socket)
        self.server_event_handler.newConnection(self.context)

    def _set_status(self, status):
        if status in (WAIT_LEN, WAIT_MESSAGE):
            self.lastRead = time.time()
        self.status = status

    def getContext(self):
        return self.context

    def connectionDestroyed(self):
        self.server_event_handler.connectionDestroyed(self.context)

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
            self.wake_up()
            return
        self.len = b''
        if len(message) == 0:
            # it was a oneway request, do not write answer
            self.message = b''
            self._set_status(WAIT_LEN)
        else:
            self.message = message
            self._set_status(SEND_ANSWER)
        self.wake_up()

    @locked
    def is_writeable(self):
        "Returns True if connection should be added to write list of select."
        return self.status == SEND_ANSWER

    # it's not necessary, but...
    @locked
    def is_readable(self, timeoutReads=None):
        """Returns True if connection should be added to read list of select.

        Closes the connection if the read timeout has expired"""
        if self.status in (WAIT_LEN, WAIT_MESSAGE):
            if timeoutReads and timeoutReads > self.lastRead:
                self.close()
                return False
            return True
        return False

    @locked
    def is_closed(self):
        "Returns True if connection is closed."
        return self.status == CLOSED

    def fileno(self):
        "Returns the file descriptor of the associated socket."
        return self.socket.fileno()

    def close(self):
        "Closes connection"
        self._set_status(CLOSED)
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
        self.use_epoll = hasattr(select, "epoll")
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

        # We may not have epoll on older systems.
        if self.use_epoll:
            poller = TSocket.ConnectionEpoll()
        else:
            poller = TSocket.ConnectionSelect()

        for fileno in self.socket.handles:
            poller.read(fileno)
        poller.read(self._read.fileno())

        readExpiration = None
        # If the last read time was older than this, close the connection.
        if self._readTimeout:
            readExpiration = time.time() - self._readTimeout

        for i, connection in self.clients.items():
            if connection.is_readable(readExpiration):
                poller.read(connection.fileno())
            if connection.is_writeable():
                poller.write(connection.fileno())
            if connection.is_closed():
                connection.connectionDestroyed()
                del self.clients[i]

        try:
            return poller.process(self.select_timeout)
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
                connection = Connection(client_socket, self.wake_up,
                    self.serverEventHandler)
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
            connection = self.clients[oob]
            connection.close()

    def close(self):
        """Closes the server."""
        for _ in xrange(self.threads):
            self.tasks.put([None, None, None, None, None])
        self.socket.close()
        self.prepared = False

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
