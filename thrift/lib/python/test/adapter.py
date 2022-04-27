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

from thrift.python.test.adapter.thrift_types import Bar, Foo


class AdapterTest(unittest.TestCase):
    def test_round_trip(self) -> None:
        now = datetime.fromtimestamp(int(time.time()))
        foo = Foo(created_at=now)
        self.assertIsInstance(foo.created_at, datetime)
        self.assertEqual(foo.created_at, now)

    def test_update(self) -> None:
        now = datetime.fromtimestamp(int(time.time()))
        foo = Foo(created_at=now)
        future = datetime.fromtimestamp(int(time.time()) + 100)
        foo = foo(created_at=future)
        self.assertEqual(foo.created_at, future)

    def test_union(self) -> None:
        now = datetime.fromtimestamp(int(time.time()))
        bar = Bar(ts=now)
        self.assertEqual(bar.ts, now)
