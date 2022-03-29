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

from thrift.conformance.protocol.thrift_types import StandardProtocol
from thrift.py3lite.conformance.any_registry import AnyRegistry

# @manual=//thrift/test/testset:testset-python-types
from thrift.test.testset import thrift_types


class AnyRegistryTest(unittest.TestCase):
    def test_round_trip(self) -> None:
        registry = AnyRegistry()
        registry.register_module(thrift_types)
        original = thrift_types.struct_map_string_i32(
            field_1={
                "Answer to the Ultimate Question of Life, the Universe, and Everything.": 42
            }
        )
        any_obj = registry.store(original, StandardProtocol.Compact)
        self.assertIsNotNone(any_obj.typeHashPrefixSha2_256)
        self.assertEqual(StandardProtocol.Compact, any_obj.protocol)
        loaded = registry.load(any_obj)
        self.assertEqual(original, loaded)
