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
from multiprocessing import Process

from thrift.py3.server import ThriftServer
from thrift.py3lite.test.services import TestServiceInterface
from thrift.py3lite.test.types import ArithmeticException, EmptyException


class Handler(TestServiceInterface):
    async def add(self, num1: int, num2: int) -> int:
        return num1 + num2

    async def divide(self, dividend: float, divisor: float) -> float:
        try:
            return dividend / divisor
        except ZeroDivisionError as e:
            raise ArithmeticException(str(e))

    async def noop(self) -> None:
        pass

    async def oops(self) -> None:
        raise EmptyException()

    async def oneway(self) -> None:
        pass

    async def surprise(self) -> None:
        raise RuntimeError("Surprise!")


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
            time.sleep(1)  # wait for server to clear the queue
            process.terminate()
            process.join()


@contextlib.asynccontextmanager
async def server_in_event_loop():
    server = ThriftServer(Handler(), ip="::1")
    serve_task = asyncio.get_event_loop().create_task(server.serve())
    addr = await server.get_address()
    try:
        yield addr
    finally:
        server.stop()
        await serve_task
