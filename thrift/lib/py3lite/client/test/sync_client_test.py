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

import asyncio
import contextlib
import tempfile
import time
import unittest
from multiprocessing import Process

from thrift.py3.server import ThriftServer
from thrift.py3lite.serializer import Protocol
from thrift.py3lite.sync_client import ClientType, get_client
from thrift.py3lite.test.lite_clients import TestService
from thrift.py3lite.test.services import TestServiceInterface


class Handler(TestServiceInterface):
    async def add(self, num1: int, num2: int) -> int:
        return num1 + num2


@contextlib.contextmanager
def server_in_another_process():
    async def start_server_async(path):
        server = ThriftServer(Handler(), path=path)
        await server.serve()

    def start_server(path):
        asyncio.run(start_server_async(path))

    with tempfile.NamedTemporaryFile() as socket:
        socket.close()
        process = Process(target=start_server, args=(socket.name,))
        process.start()
        time.sleep(1)  # wait for server to start up
        try:
            yield socket.name
        finally:
            process.kill()
            process.join()


class SyncClientTests(unittest.TestCase):
    def test_basic(self) -> None:
        with server_in_another_process() as path:
            with get_client(TestService, path=path) as client:
                sum = client.add(1, 2)
                self.assertEqual(3, sum)

    def test_client_type_and_protocol(self) -> None:
        with server_in_another_process() as path:
            with get_client(
                TestService,
                path=path,
                client_type=ClientType.THRIFT_ROCKET_CLIENT_TYPE,
                protocol=Protocol.BINARY,
            ) as client:
                sum = client.add(1, 2)
                self.assertEqual(3, sum)
