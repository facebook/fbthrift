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

import time
import unittest
from datetime import datetime

from thrift.python.test.adapter.thrift_types import Foo


class AdapterTest(unittest.TestCase):
    def test_read(self) -> None:
        ts = int(time.time())
        foo = Foo(created_at=ts)
        self.assertIsInstance(foo.created_at, datetime)
        self.assertEqual(foo.created_at, datetime.fromtimestamp(ts))
