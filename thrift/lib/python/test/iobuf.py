#!/usr/bin/env python3
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

# pyre-strict


from __future__ import annotations

import unittest

from typing import Type

import iobuf.thrift_mutable_types as mutable_types
import iobuf.thrift_types as immutable_ypes

from folly.iobuf import IOBuf

from iobuf.thrift_types import Moo as MooType

from parameterized import parameterized_class


@parameterized_class(
    ("test_types"),
    [(immutable_ypes,), (mutable_types,)],
)
class IOBufTests(unittest.TestCase):
    def setUp(self) -> None:
        """
        The `setUp` method performs these assignments with type hints to enable
        pyre when using 'parameterized'. Otherwise, Pyre cannot deduce the types
        behind `test_types`.
        """
        # pyre-ignore[16]: has no attribute `sets_types`
        self.Moo: Type[MooType] = self.test_types.Moo

    def test_get_set_struct_field(self) -> None:
        m = self.Moo(
            val=3, ptr=IOBuf(b"abcdef"), buf=IOBuf(b"xyzzy"), opt_ptr=IOBuf(b"pqr")
        )
        m2 = self.Moo(
            val=3, ptr=IOBuf(b"abcdef"), buf=IOBuf(b"xyzzy"), opt_ptr=IOBuf(b"pqr")
        )
        self.assertEqual(m, m2)
        assert m.ptr is not None
        assert m2.ptr is not None
        assert m.opt_ptr is not None
        assert m2.opt_ptr is not None

        self.assertEqual(bytes(m.ptr), bytes(m2.ptr))
        self.assertEqual(b"abcdef", bytes(m.ptr))

        self.assertEqual(bytes(m.buf), bytes(m2.buf))
        self.assertEqual(b"xyzzy", bytes(m.buf))

        self.assertEqual(bytes(m.opt_ptr), bytes(m2.opt_ptr))
        self.assertEqual(b"pqr", bytes(m.opt_ptr))
