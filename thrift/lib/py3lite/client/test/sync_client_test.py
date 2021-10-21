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

import unittest

from thrift.py3lite.serializer import Protocol
from thrift.py3lite.sync_client import ClientType, get_client
from thrift.py3lite.test.lite_clients import TestService
from thrift.py3lite.test.lite_types import ArithmeticException, EmptyException
from thrift.py3lite.test.test_server import server_in_another_process


class SyncClientTests(unittest.TestCase):
    def test_basic(self) -> None:
        with server_in_another_process() as path:
            with get_client(TestService, path=path) as client:
                self.assertEqual(3, client.add(1, 2))

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

    def test_void_return(self) -> None:
        with server_in_another_process() as path:
            with get_client(TestService, path=path) as client:
                self.assertIsNone(client.noop())

    def test_exception(self) -> None:
        with server_in_another_process() as path:
            with get_client(TestService, path=path) as client:
                self.assertAlmostEqual(2, client.divide(6, 3))
                with self.assertRaises(ArithmeticException):
                    client.divide(1, 0)

    def test_void_return_with_exception(self) -> None:
        with server_in_another_process() as path:
            with get_client(TestService, path=path) as client:
                with self.assertRaises(EmptyException):
                    client.oops()

    def test_oneway(self) -> None:
        with server_in_another_process() as path:
            with get_client(TestService, path=path) as client:
                self.assertIsNone(client.oneway())
