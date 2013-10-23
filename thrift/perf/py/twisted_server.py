#!/usr/local/bin/python2.6 -tt
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

import optparse
import sys

from twisted.internet import reactor

from apache.thrift.test.twisted.load import LoadTest
from apache.thrift.test.twisted.load_handler \
    import AsyncLoadHandler, AsyncContextLoadHandler
from thrift.protocol.TBinaryProtocol import TBinaryProtocolAcceleratedFactory
from thrift.protocol.THeaderProtocol import THeaderProtocol
from thrift.protocol.THeaderProtocol import THeaderProtocolFactory
from thrift.transport.THeaderTransport import THeaderTransport
from thrift.transport.TTwisted import ThriftServerFactory


def main():
    op = optparse.OptionParser(usage='%prog [options]', add_help_option=False)
    op.add_option('-p', '--port',
                  action='store', type='int', dest='port', default=1234,
                  help='The server port')
    op.add_option('-c', '--with-context',
                  action='store_true', help='Use the generated ContextIface')
    op.add_option('-h', '--header',
                  action='store_true', help='Use the generated ContextIface')
    op.add_option('-?', '--help',
                  action='help',
                  help='Show this help message and exit')

    (options, args) = op.parse_args()
    if args:
        op.error('trailing arguments: ' + ' '.join(args))

    if options.with_context:
        handler = AsyncContextLoadHandler()
        processor = LoadTest.ContextProcessor(handler)
    else:
        handler = AsyncLoadHandler()
        processor = LoadTest.Processor(handler)

    if options.header:
        proto_factory = THeaderProtocolFactory(True,
                          [THeaderTransport.HEADERS_CLIENT_TYPE,
                           THeaderTransport.FRAMED_DEPRECATED])
    else:
        proto_factory = TBinaryProtocolAcceleratedFactory()
    server_factory = ThriftServerFactory(processor, proto_factory)
    reactor.listenTCP(options.port, server_factory)

    print 'Serving requests on port %d...' % (options.port,)
    reactor.run()


if __name__ == '__main__':
    rc = main()
    sys.exit(rc)
