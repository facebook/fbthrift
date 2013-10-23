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

from apache.thrift.test.load import LoadTest
from apache.thrift.test.sync_load_handler import LoadHandler
from thrift.protocol.TBinaryProtocol import TBinaryProtocolAcceleratedFactory
from thrift.protocol.THeaderProtocol import THeaderProtocol
from thrift.protocol.THeaderProtocol import THeaderProtocolFactory
from thrift.transport.THeaderTransport import THeaderTransport
from thrift.transport import TSocket, TSSLSocket
from thrift.transport import TTransport
from thrift.server import TServer, TNonblockingServer, \
    TProcessPoolServer, THttpServer

def main():
    op = optparse.OptionParser(usage='%prog [options]', add_help_option=False)
    op.add_option('-p', '--port',
                  action='store', type='int', dest='port', default=1234,
                  help='The server port')
    op.add_option('-s', '--servertype',
                  action='store', type='string', dest='servertype',
                  default='TGeventServer',
                  help='Type name of server')
    op.add_option('-w', '--num_workers',
                  action='store', type='int', dest='workers', default=4,
                  help='Number of worker processes')
    op.add_option('-Q', '--max_queue_size',
                  action='store', type='int', dest='max_queue_size', default=0,
                  help='Max queue size, passed to TNonblockingServer')
    op.add_option('-h', '--header',
                  action='store_true', help='Use the generated ContextIface')
    op.add_option('-?', '--help',
                  action='help',
                  help='Show this help message and exit')

    (options, args) = op.parse_args()
    if args:
        op.error('trailing arguments: ' + ' '.join(args))

    handler = LoadHandler()
    if options.servertype == 'TGeventServer':
        # only import TGeventServer when necessary. TGeventServer calls
        # monkey_patch, which breaks other servers
        from apache.thrift.test.sync_load_handler import GeventLoadHandler
        from thrift.server import TGeventServer
        handler = GeventLoadHandler()
    processor = LoadTest.Processor(handler)

    if options.header:
        pfactory = THeaderProtocolFactory(True, \
                                          [THeaderTransport.HEADERS_CLIENT_TYPE,
                                           THeaderTransport.FRAMED_DEPRECATED,
                                           THeaderTransport.UNFRAMED_DEPRECATED,
                                           THeaderTransport.HTTP_CLIENT_TYPE])
        if options.servertype == 'TNonblockingServer':
            print 'Header transport, backwards compatible with framed'
        else:
            print 'Header transport, backwards compatible with ' + \
                'unframed, framed, http'
    else:
        if options.servertype == 'TNonblockingServer':
            print 'Framed transport'
        else:
            print 'Unframed transport'
        pfactory = TBinaryProtocolAcceleratedFactory()

    transport = TSocket.TServerSocket(options.port)
    tfactory = TTransport.TBufferedTransportFactory()
    if options.servertype == "TNonblockingServer":
        server = TNonblockingServer.TNonblockingServer(processor, transport,
                            pfactory, maxQueueSize=options.max_queue_size)
    elif options.servertype == "TProcessPoolServer":
        server = TProcessPoolServer.TProcessPoolServer(processor, transport,
                                                       tfactory,
                                                       pfactory)
        print 'Worker processes: ' + str(options.workers)
        server.setNumWorkers(options.workers)
    elif options.servertype == "TGeventServer":
        print 'Worker processes: ' + str(options.workers)
        # Gevent makes its own server transport.
        server = TGeventServer.TGeventServer(options.port,
                                             processor, None,
                                             tfactory,
                                             pfactory)
        server.setNumWorkers(options.workers)
    else:
        ServerClass = getattr(TServer, options.servertype)
        server = ServerClass(processor, transport, tfactory, pfactory)

    print 'Serving ' + options.servertype + \
        ' requests on port %d...' % (options.port,)
    server.serve()

if __name__ == '__main__':
    rc = main()
    sys.exit(rc)
