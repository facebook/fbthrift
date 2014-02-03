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

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import logging
import sys
import os
import traceback
import threading
if sys.version_info[0] >= 3:
    import queue
    Queue = queue
else:
    import Queue
import warnings

from thrift.Thrift import TProcessor, TApplicationException
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.protocol.THeaderProtocol import THeaderProtocolFactory

class TConnectionContext:
    def getPeerName(self):
        """Gets the address of the client.

        Returns:
          The equivalent value of socket.getpeername() on the client socket
        """
        raise NotImplementedError


class TRpcConnectionContext(TConnectionContext):
    """Connection context class for thrift RPC calls"""
    def __init__(self, client_socket, iprot=None, oprot=None):
        """Initializer.

        Arguments:
          client_socket: the TSocket to the client
        """
        self._client_socket = client_socket
        self.iprot = iprot
        self.oprot = oprot

    def setProtocols(self, iprot, oprot):
        self.iprot = iprot
        self.oprot = oprot

    def getPeerName(self):
        """Gets the address of the client.

        Returns:
          Same value as socket.peername() for the TSocket
        """
        return self._client_socket.getPeerName()


class TServerEventHandler:
    """Event handler base class.

    Override selected methods on this class to implement custom event handling
    """
    def preServe(self, address):
        """Called before the server begins.

        Arguments:
          address: the address that the server is listening on
        """
        pass

    def newConnection(self, context):
        """Called when a client has connected and is about to begin processing.

        Arguments:
          context: instance of TRpcConnectionContext
        """
        pass

    def clientBegin(self, iprot, oprot):
        """Deprecated: Called when a new connection is made to the server.

        For all servers other than TNonblockingServer, this function is called
        whenever newConnection is called and vice versa.  This is the old-style
        for event handling and is not supported for TNonblockingServer. New
        code should always use the newConnection method.
        """
        pass

    def connectionDestroyed(self, context):
        """Called when a client has finished request-handling.

        Arguments:
          context: instance of TRpcConnectionContext
        """
        pass


class TServer:

    """Base interface for a server, which must have a serve method."""

    """ constructors for all servers:
    1) (processor, serverTransport)
    2) (processor, serverTransport, transportFactory, protocolFactory)
    3) (processor, serverTransport,
        inputTransportFactory, outputTransportFactory,
        inputProtocolFactory, outputProtocolFactory)

        Optionally, the handler can be passed instead of the processor,
        and a processor will be created automatically:

    4) (handler, serverTransport)
    5) (handler, serverTransport, transportFacotry, protocolFactory)
    6) (handler, serverTransport,
        inputTransportFactory, outputTransportFactory,
        inputProtocolFactory, outputProtocolFactory)

        The attribute serverEventHandler (default: None) receives
        callbacks for various events in the server lifecycle.  It should
        be set to an instance of TServerEventHandler.

        """
    def __init__(self, *args):
        if (len(args) == 2):
            self.__initArgs__(args[0], args[1],
                              TTransport.TTransportFactoryBase(),
                              TTransport.TTransportFactoryBase(),
                              TBinaryProtocol.TBinaryProtocolFactory(),
                              TBinaryProtocol.TBinaryProtocolFactory())
        elif (len(args) == 4):
            self.__initArgs__(args[0], args[1], args[2], args[2], args[3],
                    args[3])
        elif (len(args) == 6):
            self.__initArgs__(args[0], args[1], args[2], args[3], args[4],
                    args[5])

    def __initArgs__(self, processor, serverTransport,
                     inputTransportFactory, outputTransportFactory,
                     inputProtocolFactory, outputProtocolFactory):
        self.processor = self._getProcessor(processor)
        self.serverTransport = serverTransport
        self.inputTransportFactory = inputTransportFactory
        self.outputTransportFactory = outputTransportFactory
        self.inputProtocolFactory = inputProtocolFactory
        self.outputProtocolFactory = outputProtocolFactory

        self.serverEventHandler = TServerEventHandler()

    def _getProcessor(self, processor):
        """ Check if a processor is really a processor, or if it is a handler
            auto create a processor for it """
        if isinstance(processor, TProcessor):
            return processor
        elif hasattr(processor, "_processor_type"):
            handler = processor
            return handler._processor_type(handler)
        else:
            raise TApplicationException(
                    message="Could not detect processor type")

    def setServerEventHandler(self, handler):
        self.serverEventHandler = handler

    def _clientBegin(self, context, iprot, oprot):
        self.serverEventHandler.newConnection(context)
        self.serverEventHandler.clientBegin(iprot, oprot)

    def handle(self, client):
        itrans = self.inputTransportFactory.getTransport(client)
        otrans = self.outputTransportFactory.getTransport(client)
        iprot = self.inputProtocolFactory.getProtocol(itrans)

        if isinstance(self.inputProtocolFactory, THeaderProtocolFactory):
            oprot = iprot
        else:
            oprot = self.outputProtocolFactory.getProtocol(otrans)

        context = TRpcConnectionContext(client, iprot, oprot)
        self._clientBegin(context, iprot, oprot)

        try:
            while True:
                self.processor.process(iprot, oprot, context)
        except TTransport.TTransportException as tx:
            pass
        except Exception as x:
            logging.exception(x)

        self.serverEventHandler.connectionDestroyed(context)
        itrans.close()
        otrans.close()

    def serve(self):
        pass

class TSimpleServer(TServer):

    """Simple single-threaded server that just pumps around one transport."""

    def __init__(self, *args):
        warnings.warn("TSimpleServer is deprecated. Please use one of "
                      "Nonblocking, Twisted, or Gevent server instead.",
                      DeprecationWarning)
        TServer.__init__(self, *args)

    def serve(self):
        self.serverTransport.listen()
        for name in self.serverTransport.getSocketNames():
            self.serverEventHandler.preServe(name)
        while True:
            client = self.serverTransport.accept()
            self.handle(client)


class TThreadedServer(TServer):

    """Threaded server that spawns a new thread per each connection."""

    def __init__(self, *args, **kwargs):
        TServer.__init__(self, *args)
        self.daemon = kwargs.get("daemon", False)

    def serve(self):

        self.serverTransport.listen()
        for name in self.serverTransport.getSocketNames():
            self.serverEventHandler.preServe(name)
        while True:
            try:
                client = self.serverTransport.accept()
                t = threading.Thread(target=self.handle, args=(client,))
                t.daemon = self.daemon
                t.start()
            except KeyboardInterrupt:
                raise
            except Exception as x:
                logging.exception(x)


class TThreadPoolServer(TServer):

    """Server with a fixed size pool of threads which service requests."""

    def __init__(self, *args, **kwargs):
        warnings.warn("TThreadPoolServer is deprecated. Please use one of "
                      "Nonblocking, Twisted, or Gevent server instead.",
                      DeprecationWarning)
        TServer.__init__(self, *args)
        queue_size = kwargs.get("queueSize", 0)
        self.clients = Queue.Queue(queue_size)
        self.threads = 10
        self.daemon = kwargs.get("daemon", False)
        self.timeout = kwargs.get("timeout", None)

    def setNumThreads(self, num):
        """Set the number of worker threads that should be created"""
        self.threads = num

    def serveThread(self):
        """
        Loop around getting clients from the shared queue and process them.
        """
        while True:
            try:
                client = self.clients.get()
                if self.timeout:
                    client.setTimeout(self.timeout)
                self.handle(client)
            except Exception as x:
                logging.exception(x)

    def serve(self):
        """
        Start a fixed number of worker threads and put client into a queue
        """
        for i in range(self.threads):
            try:
                t = threading.Thread(target=self.serveThread)
                t.daemon = self.daemon
                t.start()
            except Exception as x:
                logging.exception(x)

        # Pump the socket for clients
        self.serverTransport.listen()
        for name in self.serverTransport.getSocketNames():
            self.serverEventHandler.preServe(name)
        while True:
            client = None
            try:
                client = self.serverTransport.accept()
                self.clients.put(client)
            except Exception as x:
                logging.exception(x)
                if client:
                    itrans = self.inputTransportFactory.getTransport(client)
                    otrans = self.outputTransportFactory.getTransport(client)
                    itrans.close()
                    otrans.close()


class TForkingServer(TServer):

    """A Thrift server that forks a new process for each request"""
    """
    This is more scalable than the threaded server as it does not cause
    GIL contention.

    Note that this has different semantics from the threading server.
    Specifically, updates to shared variables will no longer be shared.
    It will also not work on windows.

    This code is heavily inspired by SocketServer.ForkingMixIn in the
    Python stdlib.
    """

    def __init__(self, *args):
        TServer.__init__(self, *args)
        self.children = []

    def serve(self):
        def tryClose(file):
            try:
                file.close()
            except IOError as e:
                logging.warning(e, exc_info=True)

        self.serverTransport.listen()
        for name in self.serverTransport.getSocketNames():
            self.serverEventHandler.preServe(name)
        while True:
            client = self.serverTransport.accept()
            try:

                itrans = self.inputTransportFactory.getTransport(client)
                otrans = self.outputTransportFactory.getTransport(client)

                iprot = self.inputProtocolFactory.getProtocol(itrans)

                if isinstance(self.inputProtocolFactory,
                        THeaderProtocolFactory):
                    oprot = iprot
                else:
                    oprot = self.outputProtocolFactory.getProtocol(otrans)

                context = TRpcConnectionContext(client, iprot, oprot)
                self._clientBegin(context, iprot, oprot)

                pid = os.fork()

                if pid:  # parent
                    # add before collect, otherwise you race w/ waitpid
                    self.children.append(pid)
                    self._collectChildren()

                    # Parent must close socket or the connection may not get
                    # closed promptly
                    tryClose(itrans)
                    tryClose(otrans)

                else:

                    ecode = 0
                    try:
                        try:
                            while True:
                                self.processor.process(iprot, oprot, context)
                        except TTransport.TTransportException as tx:
                            pass
                        except Exception as e:
                            logging.exception(e)
                            ecode = 1
                    finally:
                        self.serverEventHandler.connectionDestroyed(context)
                        tryClose(itrans)
                        tryClose(otrans)

                    os._exit(ecode)

            except TTransport.TTransportException as tx:
                pass
            except Exception as x:
                logging.exception(x)

    def _collectChildren(self):
        while self.children:
            try:
                pid, status = os.waitpid(0, os.WNOHANG)
            except os.error:
                pid = None

            if pid:
                self.children.remove(pid)
            else:
                break
