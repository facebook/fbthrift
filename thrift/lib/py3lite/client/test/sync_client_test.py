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
from thrift.py3lite.sync_client import ClientType, SyncClient, get_client

# @manual=//thrift/lib/py3lite/client/test:test_service-py3lite-types
from thrift.py3lite.test.lite_types import (
    _fbthrift_TestService_add_args,
    _fbthrift_TestService_add_result,
)
from thrift.py3lite.test.services import TestServiceInterface


# TODO: replace with codegen-ed client
class Client(SyncClient):
    def __init__(self, channel):
        super(Client, self).__init__(channel, "TestService")

    def add(self, num1, num2):
        return self._send_request(
            "add",
            _fbthrift_TestService_add_args(num1=num1, num2=num2),
            _fbthrift_TestService_add_result,
        ).success


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
            with get_client(Client, path=path) as client:
                sum = client.add(1, 2)
                self.assertEqual(3, sum)

    def test_client_type_and_protocol(self) -> None:
        with server_in_another_process() as path:
            with get_client(
                Client,
                path=path,
                client_type=ClientType.THRIFT_ROCKET_CLIENT_TYPE,
                protocol=Protocol.BINARY,
            ) as client:
                sum = client.add(1, 2)
                self.assertEqual(3, sum)
