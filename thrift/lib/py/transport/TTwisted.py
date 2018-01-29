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

from zope.interface import implements, Interface, Attribute
from struct import unpack
from twisted.internet.protocol import Protocol, ServerFactory, ClientFactory, \
    connectionDone
from twisted.internet import defer
from twisted.protocols import basic
from twisted.python import log
from thrift.protocol.THeaderProtocol import THeaderProtocolFactory
from thrift.server import TServer
from thrift.transport import TTransport

import sys
if sys.version_info[0] >= 3:
    from io import StringIO
else:
    from cStringIO import StringIO


class TMessageSenderTransport(TTransport.TTransportBase):

    def __init__(self):
        self.__wbuf = StringIO()

    def write(self, buf):
        self.__wbuf.write(buf)

    def flush(self):
        msg = self.__wbuf.getvalue()
        self.__wbuf = StringIO()
        self.sendMessage(msg)

    def sendMessage(self, message):
        raise NotImplementedError


class TCallbackTransport(TMessageSenderTransport):

    def __init__(self, func):
        TMessageSenderTransport.__init__(self)
        self.func = func

    def sendMessage(self, message):
        self.func(message)


class ThriftClientProtocol(basic.Int32StringReceiver):
    MAX_LENGTH = 1 << 24

    def __init__(self, client_class, iprot_factory, oprot_factory=None):
        self._client_class = client_class
        self._iprot_factory = iprot_factory
        if oprot_factory is None:
            self._oprot_factory = iprot_factory
        else:
            self._oprot_factory = oprot_factory

        self._errormsg = None
        self.recv_map = {}
        self.started = defer.Deferred()

    def dispatch(self, msg):
        self.sendString(msg)

    def connectionMade(self):
        tmo = TCallbackTransport(self.dispatch)
        self.client = self._client_class(tmo, self._oprot_factory)
        self.started.callback(self.client)

    def connectionLost(self, reason=connectionDone):
        if sys.version_info[0] >= 3:
            client_req_iter = self.client._reqs.items()
        else:
            client_req_iter = self.client._reqs.items()
        for _, v in client_req_iter:
            tex = TTransport.TTransportException(
                type=TTransport.TTransportException.END_OF_FILE,
                message=self._errormsg or 'Connection closed')
            v.errback(tex)

    def stringReceived(self, frame):
        tr = TTransport.TMemoryBuffer(frame)
        iprot = self._iprot_factory.getProtocol(tr)
        (fname, mtype, rseqid) = iprot.readMessageBegin()

        try:
            method = self.recv_map[fname]
        except KeyError:
            method = getattr(self.client, 'recv_' + fname)
            self.recv_map[fname] = method

        method(iprot, mtype, rseqid)

    def lengthLimitExceeded(self, length):
        self._errormsg = 'Received frame too large (%s > %s)' % (
            length, self.MAX_LENGTH)
        self.transport.loseConnection()


class TwistedRpcConnectionContext(TServer.TConnectionContext):

    def __init__(self, client_socket):
        self._client_socket = client_socket

    def getPeerName(self):
        return self._client_socket.getpeername()


class ThriftServerProtocol(basic.Int32StringReceiver):
    MAX_LENGTH = 1 << 24

    def dispatch(self, msg):
        self.sendString(msg)

    def processError(self, error):
        self.transport.loseConnection()

    def processOk(self, _, tmo):
        msg = tmo.getvalue()

        if len(msg) > 0:
            self.dispatch(msg)

    def stringReceived(self, frame):
        tmi = TTransport.TMemoryBuffer(frame)
        tmo = TTransport.TMemoryBuffer()

        iprot = self.factory.iprot_factory.getProtocol(tmi)
        oprot = self.factory.oprot_factory.getProtocol(tmo)

        server_ctx = TwistedRpcConnectionContext(self.transport.socket)
        d = self.factory.processor.process(iprot, oprot, server_ctx)
        d.addCallbacks(self.processOk, self.processError,
            callbackArgs=(tmo,))


class ThriftHeaderServerProtocol(Protocol):
    MAX_LENGTH = 1 << 24
    recvd = b""

    def dataReceived(self, recvd):
        self.recvd = self.recvd + recvd
        while len(self.recvd) >= 4:
            length, = unpack(b"!I", self.recvd[:4])
            if length > self.MAX_LENGTH:
                self.transport.loseConnection()
                return
            if len(self.recvd) < length + 4:
                break
            packet = self.recvd[0:4 + length]
            self.recvd = self.recvd[4 + length:]
            self.stringReceived(packet)

    def processError(self, error):
        self.transport.loseConnection()

    def processOk(self, _, tmo):
        msg = tmo.getvalue()

        if len(msg) > 0:
            # HeaderTransport will have already done msg length checking,
            # and already adds the frame size.  Write directly.
            self.transport.write(msg)

    def stringReceived(self, frame):
        tmi = TTransport.TMemoryBuffer(frame)
        iprot = self.factory.iprot_factory.getProtocol(tmi)
        oprot = iprot
        tmo = tmi

        server_ctx = TwistedRpcConnectionContext(self.transport.socket)
        d = self.factory.processor.process(iprot, oprot, server_ctx)
        d.addCallbacks(self.processOk, self.processError,
            callbackArgs=(tmo,))


class IThriftServerFactory(Interface):

    processor = Attribute("Thrift processor")

    iprot_factory = Attribute("Input protocol factory")

    oprot_factory = Attribute("Output protocol factory")


class IThriftClientFactory(Interface):

    client_class = Attribute("Thrift client class")

    iprot_factory = Attribute("Input protocol factory")

    oprot_factory = Attribute("Output protocol factory")


class ThriftServerFactory(ServerFactory):

    implements(IThriftServerFactory)

    protocol = ThriftServerProtocol

    def __init__(self, processor, iprot_factory, oprot_factory=None):
        self.processor = processor
        self.iprot_factory = iprot_factory
        if oprot_factory is None:
            self.oprot_factory = iprot_factory
        else:
            self.oprot_factory = oprot_factory

        if isinstance(iprot_factory, THeaderProtocolFactory):
            self.protocol = ThriftHeaderServerProtocol

class ThriftClientFactory(ClientFactory):

    implements(IThriftClientFactory)

    protocol = ThriftClientProtocol

    def __init__(self, client_class, iprot_factory, oprot_factory=None):
        self.client_class = client_class
        self.iprot_factory = iprot_factory
        if oprot_factory is None:
            self.oprot_factory = iprot_factory
        else:
            self.oprot_factory = oprot_factory

    def buildProtocol(self, addr):
        p = self.protocol(self.client_class, self.iprot_factory,
            self.oprot_factory)
        p.factory = self
        return p
