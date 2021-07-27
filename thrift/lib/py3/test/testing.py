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

from unittest import IsolatedAsyncioTestCase
from unittest.mock import patch, Mock

import thrift.py3  # @manual
from testing.clients import TestingService
from thrift.py3.testing import mock_client


class TestTesting(IsolatedAsyncioTestCase):
    @patch("thrift.py3.get_client")
    async def test_mock_client(self, mock_get_client: Mock) -> None:
        mock_get_client.return_value = mock_client(TestingService)
        mock_get_client.return_value.getName.return_value = "mock_client"

        async with thrift.py3.get_client(
            TestingService, host="localhost", port=8
        ) as client:
            await client.getName()
            self.assertEqual(await client.getName(), "mock_client")
            with self.assertRaises(AttributeError):
                # pyre-ignore
                await client.unknownmethod()

        self.assertTrue(mock_get_client.return_value.getName.called)
        self.assertEqual(mock_get_client.return_value.getName.call_count, 2)
