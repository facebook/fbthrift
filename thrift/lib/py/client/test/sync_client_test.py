# Copyright (c) Meta Platforms, Inc. and affiliates.
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

from thrift.py.client.sync_client import SyncClient
from thrift.py.client.sync_client_factory import get_client

# @manual=//thrift/lib/py3lite/client/test:test_service-py
from thrift.py.test.TestService import add_args, add_result
from thrift.py3lite.test.test_server import server_in_another_process


class TestServiceClient(SyncClient):
    def add(self, num1: int, num2: int) -> int:
        result = self._send_request(
            "TestService", "add", add_args(num1=num1, num2=num2), add_result
        )
        return result.success


class SyncClientTests(unittest.TestCase):
    def test_basic(self) -> None:
        with server_in_another_process() as path:
            with get_client(TestServiceClient, path=path) as client:
                self.assertEqual(3, client.add(1, 2))
