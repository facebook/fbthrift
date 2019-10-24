# Copyright (c) Facebook, Inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import socket
import threading
import trollius as asyncio
import unittest

from contextlib import contextmanager
from trollius import (
    From,
    Return,
)
from six import itervalues

import thrift
thrift.trollius = True
from thrift_asyncio.tutorial import Calculator as AsyncCalculator
from thrift.protocol.THeaderProtocol import THeaderProtocolFactory
from thrift.server.test.handler import CalculatorHandler
from thrift.server.TNonblockingServer import TNonblockingServer
from thrift.transport.TSocket import TServerSocket
from thrift.util.async_common import THEADER_CLIENT_TYPES
from thrift.util.trollius import create_client
from tutorial import Calculator


class TTrolliusServerTest(unittest.TestCase):

    def _get_listening_port(self, server):
        ports = [
            handle.getsockname()[1]
            for handle in itervalues(server.socket.handles)
            if handle.family == socket.AF_INET6
        ]
        if ports:
            return ports[0]
        return None

    def _create_server(self, port=0):
        handler = CalculatorHandler()
        transport = TServerSocket(port)
        pfactory = THeaderProtocolFactory(client_types=THEADER_CLIENT_TYPES)
        return TNonblockingServer(
            Calculator.Processor(handler),
            transport,
            inputProtocolFactory=pfactory,
            outputProtocolFactory=pfactory,
            threads=1,
        )

    @contextmanager
    def _running_server(self, port=0):
        server = self._create_server(port)
        server.prepare()
        t = threading.Thread(target=server.serve)
        t.start()
        try:
            yield server
        finally:
            server.stop()
            server.close()
            t.join()

    @asyncio.coroutine
    def _call_server_in_loop(self, loop, port):
        client = yield From(create_client(
            AsyncCalculator.Client,
            host="localhost",
            port=port,
            loop=loop,
        ))
        with client as c:
            add_result = yield From(c.add(1, 2))
        raise Return(add_result)

    def _test_using_event_loop(self, loop):
        with self._running_server() as server:
            port = self._get_listening_port(server)
            add_result = loop.run_until_complete(
                self._call_server_in_loop(loop, port)
            )
            self.assertEqual(42, add_result)

    def test_default_event_loop(self):
        loop = asyncio.get_event_loop()
        loop.set_debug(True)
        self._test_using_event_loop(loop)

    def test_custom_event_loop(self):
        loop = asyncio.new_event_loop()
        loop.set_debug(True)
        self._test_using_event_loop(loop)
