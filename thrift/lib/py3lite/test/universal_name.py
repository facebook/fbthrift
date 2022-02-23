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

from thrift.py3lite.universal_name import (
    validate_universal_hash,
    validate_universal_hash_bytes,
    validate_universal_name,
    UniversalHashAlgorithm,
)


class UnionTests(unittest.TestCase):
    def test_validate_universal_name(self) -> None:
        validate_universal_name("foo.com/my/type")
        with self.assertRaises(ValueError):
            validate_universal_name("foo.com/my/type#1")

    def test_validate_universal_hash(self) -> None:
        validate_universal_hash(UniversalHashAlgorithm.Sha2_256, b"a" * 32, 8)
        with self.assertRaises(ValueError):
            validate_universal_hash(UniversalHashAlgorithm.Sha2_256, b"b" * 1, 8)
        with self.assertRaises(ValueError):
            validate_universal_hash(UniversalHashAlgorithm.Sha2_256, b"c" * 33, 8)

    def test_validate_universal_hash_bytes(self) -> None:
        validate_universal_hash_bytes(32, 8)
        with self.assertRaises(ValueError):
            validate_universal_hash_bytes(1, 8)
