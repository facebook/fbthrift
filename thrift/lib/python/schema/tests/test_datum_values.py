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

# The codec's value representation

from __future__ import annotations

import math
import pickle
import unittest
from collections.abc import Callable

from thrift.lib.python.schema._datum import EMPTY_UNION, FrozenMap, FrozenSet, UNSET
from thrift.lib.python.schema._errors import EncodeError
from thrift.lib.python.schema._value_checks import (
    check_binary,
    check_bool,
    check_double,
    check_float32,
    check_int,
    check_string,
)


class FrozenMapTest(unittest.TestCase):
    def test_equality_ignores_order_and_matches_dict(self) -> None:
        self.assertEqual(FrozenMap({1: "a", 2: "b"}), FrozenMap({2: "b", 1: "a"}))
        self.assertEqual(FrozenMap({1: "a"}), {1: "a"})
        self.assertNotEqual(FrozenMap({1: "a"}), FrozenMap({1: "b"}))

    def test_hash_ignores_order(self) -> None:
        first = FrozenMap({1: "a", 2: "b"})
        second = FrozenMap({2: "b", 1: "a"})
        self.assertEqual(hash(first), hash(second))
        self.assertEqual(len({first, second}), 1)

    def test_iterates_in_insertion_order(self) -> None:
        value = FrozenMap({"z": 1, "a": 2, "m": 3})
        self.assertEqual(list(value), ["z", "a", "m"])
        self.assertEqual(list(value.items()), [("z", 1), ("a", 2), ("m", 3)])

    def test_nests_as_keys_and_elements(self) -> None:
        inner = FrozenMap({1: (2, 3)})
        outer = FrozenMap({inner: FrozenSet([inner])})
        self.assertEqual(outer[FrozenMap({1: (2, 3)})], FrozenSet([inner]))

    def test_rejects_mutation(self) -> None:
        value = FrozenMap({1: 2})
        mutations: list[Callable[[], object]] = [
            lambda: value.__setitem__(3, 4),
            lambda: value.__delitem__(1),
            value.clear,
            lambda: value.pop(1),
            value.popitem,
            lambda: value.setdefault(5, 6),
            lambda: value.update({7: 8}),
            lambda: value.__ior__({9: 10}),
        ]
        for mutate in mutations:
            with self.assertRaises(TypeError):
                mutate()
        self.assertEqual(value, {1: 2})

    def test_survives_pickle_and_copy(self) -> None:
        value = FrozenMap({"b": 1, "a": 2})
        restored = pickle.loads(pickle.dumps(value))
        self.assertIsInstance(restored, FrozenMap)
        self.assertEqual(list(restored.items()), [("b", 1), ("a", 2)])
        self.assertIsInstance(value.copy(), FrozenMap)

    def test_repr(self) -> None:
        self.assertEqual(repr(FrozenMap({1: "a"})), "FrozenMap({1: 'a'})")


class FrozenSetTest(unittest.TestCase):
    def test_iterates_in_insertion_order(self) -> None:
        self.assertEqual(list(FrozenSet(["z", "a", "m"])), ["z", "a", "m"])

    def test_duplicates_keep_first_position(self) -> None:
        self.assertEqual(list(FrozenSet([3, 1, 3, 2, 1])), [3, 1, 2])

    def test_equality_and_hash_match_frozenset(self) -> None:
        self.assertEqual(FrozenSet([1, 2]), FrozenSet([2, 1]))
        self.assertEqual(FrozenSet([1, 2]), frozenset({1, 2}))
        self.assertEqual(hash(FrozenSet([1, 2])), hash(frozenset({2, 1})))

    def test_nests_as_elements_and_keys(self) -> None:
        inner = FrozenSet([(1, 2)])
        outer = FrozenSet([inner, FrozenMap({inner: inner})])
        self.assertIn(FrozenSet([(1, 2)]), outer)

    def test_has_no_mutating_methods(self) -> None:
        for name in ("add", "discard", "remove", "update", "clear"):
            self.assertFalse(hasattr(FrozenSet(), name), name)

    def test_survives_pickle(self) -> None:
        restored = pickle.loads(pickle.dumps(FrozenSet(["b", "a"])))
        self.assertIsInstance(restored, FrozenSet)
        self.assertEqual(list(restored), ["b", "a"])

    def test_repr(self) -> None:
        self.assertEqual(repr(FrozenSet([2, 1])), "FrozenSet([2, 1])")


class UnsetTest(unittest.TestCase):
    def test_is_a_distinct_hashable_sentinel(self) -> None:
        self.assertIsNot(UNSET, None)
        self.assertEqual(repr(UNSET), "UNSET")
        self.assertEqual(len({UNSET, UNSET}), 1)
        self.assertEqual(pickle.loads(pickle.dumps(UNSET)), UNSET)

    def test_empty_union_is_a_different_sentinel(self) -> None:
        self.assertIsNot(EMPTY_UNION, UNSET)
        self.assertNotEqual(EMPTY_UNION, UNSET)
        self.assertEqual(repr(EMPTY_UNION), "EMPTY_UNION")
        self.assertIs(pickle.loads(pickle.dumps(EMPTY_UNION)), EMPTY_UNION)


class PrimitiveChecksTest(unittest.TestCase):
    def test_accepts_values_without_coercion(self) -> None:
        cases: list[tuple[Callable[[object], object], object]] = [
            (check_bool, True),
            (lambda v: check_int(v, 8, "byte"), -128),
            (lambda v: check_int(v, 16, "i16"), 2**15 - 1),
            (lambda v: check_int(v, 32, "i32"), -(2**31)),
            (lambda v: check_int(v, 64, "i64"), 2**63 - 1),
            (check_float32, 0.5),
            (check_float32, math.inf),
            (check_float32, math.nan),
            (check_double, 0.1),
            (check_string, "☃"),
            (check_binary, b"\x00\xff"),
        ]
        for check, value in cases:
            with self.subTest(value=value):
                self.assertIs(check(value), value)

    def test_rejects_wrong_types_and_out_of_range_values(self) -> None:
        cases: list[tuple[Callable[[object], object], object]] = [
            (check_bool, 1),
            (lambda v: check_int(v, 32, "i32"), True),
            (lambda v: check_int(v, 8, "byte"), 128),
            (lambda v: check_int(v, 16, "i16"), -(2**15) - 1),
            (lambda v: check_int(v, 32, "i32"), 2**31),
            (lambda v: check_int(v, 64, "i64"), 2**63),
            (lambda v: check_int(v, 64, "i64"), 1.0),
            (check_double, 1),
            (check_float32, 0.1),
            (check_float32, 1e39),
            (check_string, "\ud800"),
            (check_string, b"x"),
            (check_binary, bytearray(b"x")),
            (check_binary, "x"),
        ]
        for check, value in cases:
            with self.subTest(value=value):
                with self.assertRaises(EncodeError):
                    check(value)
