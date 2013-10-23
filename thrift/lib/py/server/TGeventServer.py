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
import multiprocessing
from multiprocessing import  Process
import gevent
from gevent.server import StreamServer

from .TServer import TServer
from thrift.transport.TTransport import TTransportException
from thrift.transport.TSocket import TSocket
from thrift.protocol.THeaderProtocol import THeaderProtocolFactory

from gevent import monkey
monkey.patch_all()

class TGeventServer(TServer):

    """
    Server with a fixed size pool of worker subprocesses which service requests.
    Note that if you need shared state between the handlers - it's up to you!
    Written by Dvir Volk, doat.com
    """

    def __init__(self, port, *args):
        TServer.__init__(self, *args)
        self.port = port
        self.numWorkers = multiprocessing.cpu_count()
        self.workers = []
        self.postForkCallback = None

    def setPostForkCallback(self, callback):
        if not callable(callback):
            raise TypeError("This is not a callback!")
        self.postForkCallback = callback

    def setNumWorkers(self, num):
        """Set the number of worker threads that should be created"""
        self.numWorkers = num

    def serveClient(self, socket, address):
        """Process input/output from a client for as long as possible"""
        client = TSocket()
        client.setHandle(socket)
        itrans = self.inputTransportFactory.getTransport(client)
        otrans = self.outputTransportFactory.getTransport(client)
        iprot = self.inputProtocolFactory.getProtocol(itrans)
        if isinstance(self.inputProtocolFactory, THeaderProtocolFactory):
            oprot = iprot
        else:
            oprot = self.outputProtocolFactory.getProtocol(otrans)

        try:
            while True:
                self.processor.process(iprot, oprot)
        except TTransportException as tx:
            pass
        except Exception as x:
            logging.exception(x)

        itrans.close()
        otrans.close()

    def serve_forever(self):
        if self.postForkCallback:
            self.postForkCallback()
        while True:
            try:
                self.server.serve_forever()
            except (KeyboardInterrupt, SystemExit):
                return 0
            except Exception as x:
                logging.exception(x)

    def serve(self):
        """Start a fixed number of worker threads and put client into a queue"""

        self.server = StreamServer(('', self.port), self.serveClient)
        # Temporary patch for gevent 0.13.x
        # Remove pre_start when we are fully on gevent 1.0
        if gevent.version_info[0] == 0:
            self.server.pre_start()
        else:
            self.server.init_socket()

        print('Starting %s workers' % self.numWorkers)
        for i in range(self.numWorkers - 1):  # Current process also serves
            p = Process(target=self.serve_forever)
            self.workers.append(p)
            p.start()

        self.serve_forever()

    def stop(self):
        for worker in self.workers:
            worker.terminate()
        self.server.stop()
