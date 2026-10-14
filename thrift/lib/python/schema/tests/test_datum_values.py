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

import thrift.lib.python.schema.tests.codec_test.thrift_types as fixtures
from thrift.lib.python.schema._datum import (
    check_value,
    copy_value,
    freeze_key,
    FrozenMap,
    UNSET,
    values_equal,
)
from thrift.lib.python.schema._errors import EncodeError
from thrift.lib.python.schema._value_checks import (
    check_binary,
    check_bool,
    check_double,
    check_float32,
    check_int,
    check_string,
)
from thrift.lib.python.schema.tests.datum_testing import (
    DOUBLE,
    I32,
    struct_type_of,
    type_ref_of,
)
from thrift.lib.python.schema.type_system import (
    ListTypeRef,
    MapTypeRef,
    SetTypeRef,
    TypeRef,
)

POINT = struct_type_of(fixtures.Point)
CHOICE = type_ref_of(fixtures.Choice)


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
        outer = FrozenMap({inner: FrozenMap({inner: None})})
        self.assertEqual(outer[FrozenMap({1: (2, 3)})], FrozenMap({inner: None}))

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


class UnsetTest(unittest.TestCase):
    def test_is_a_distinct_hashable_sentinel(self) -> None:
        self.assertIsNot(UNSET, None)
        self.assertEqual(repr(UNSET), "UNSET")
        self.assertEqual(len({UNSET, UNSET}), 1)
        self.assertEqual(pickle.loads(pickle.dumps(UNSET)), UNSET)


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


class OwnershipTest(unittest.TestCase):
    def test_construction_isolates_caller_containers(self) -> None:
        inner = [1, 2]
        source = [inner, inner]
        value = copy_value(source, ListTypeRef(ListTypeRef(I32)))
        assert isinstance(value, list)
        first, second = value
        assert isinstance(first, list) and isinstance(second, list)
        inner.append(3)
        first.append(4)
        self.assertEqual(first, [1, 2, 4])
        self.assertEqual(second, [1, 2])
        self.assertIsNot(first, second)

    def test_copy_isolates_nested_map_values(self) -> None:
        type_ref = MapTypeRef(I32, ListTypeRef(I32))
        source = copy_value({1: [2]}, type_ref)
        copied = copy_value(source, type_ref)
        assert isinstance(source, dict) and isinstance(copied, dict)
        source_child = source[1]
        assert isinstance(source_child, list)
        source_child.append(3)
        self.assertEqual(copied, {1: [2]})


class FrozenEntriesTest(unittest.TestCase):
    def test_list_keys_and_owned_values_are_isolated(self) -> None:
        key = [1, 2]
        payload = [3]
        value = {
            freeze_key(key, ListTypeRef(I32)): copy_value(payload, ListTypeRef(I32))
        }
        assert isinstance(value, dict)
        key.append(4)
        payload.append(5)
        self.assertEqual(value, {(1, 2): [3]})
        check_value(value, MapTypeRef(ListTypeRef(I32), ListTypeRef(I32)))

    def test_freezing_a_complex_key_recursively_isolates_storage(self) -> None:
        type_ref = MapTypeRef(I32, ListTypeRef(I32))
        source = copy_value({1: [2]}, type_ref)
        frozen = freeze_key(source, type_ref)
        assert isinstance(source, dict)
        child = source[1]
        assert isinstance(child, list)
        child.append(3)
        self.assertEqual(frozen, FrozenMap({1: (2,)}))
        self.assertEqual(hash(frozen), hash(FrozenMap({1: (2,)})))

    def test_set_elements_are_frozen_and_ordered(self) -> None:
        source = [[2], [1], [2]]
        value = dict.fromkeys(
            freeze_key(element, ListTypeRef(I32)) for element in source
        )
        assert isinstance(value, dict)
        source[0].append(3)
        self.assertEqual(list(value), [(2,), (1,)])
        self.assertEqual(
            freeze_key(value, SetTypeRef(ListTypeRef(I32))),
            FrozenMap({(2,): None, (1,): None}),
        )

    def test_validation_precedes_duplicate_collapse(self) -> None:
        with self.assertRaises(EncodeError):
            dict.fromkeys(freeze_key(element, I32) for element in [1, True])
        with self.assertRaises(EncodeError):
            {
                freeze_key(key, I32): copy_value(value, I32)
                for key, value in [(1, 2), (True, 3)]
            }

    def test_nan_keeps_stored_representative(self) -> None:
        nan = math.nan
        value = dict.fromkeys(freeze_key(element, DOUBLE) for element in [nan, nan])
        assert isinstance(value, dict)
        self.assertEqual(len(value), 1)
        self.assertIs(next(iter(value)), nan)


class FrozenCopyTest(unittest.TestCase):
    def test_copies_frozen_storage_into_owned_storage(self) -> None:
        cases: list[tuple[object, TypeRef, object]] = [
            ((1, 2), POINT, [1, 2]),
            ((), CHOICE, []),
            ((3, (1, 2)), CHOICE, [3, [1, 2]]),
            (((1,), ()), ListTypeRef(ListTypeRef(I32)), [[1], []]),
            (FrozenMap({1: (2,)}), MapTypeRef(I32, ListTypeRef(I32)), {1: [2]}),
        ]
        for frozen, type_ref, owned in cases:
            with self.subTest(frozen=frozen):
                copied = copy_value(frozen, type_ref, frozen=True)
                self.assertEqual(copied, owned)
                check_value(copied, type_ref)

    def test_copies_are_independent_and_share_frozen_keys(self) -> None:
        type_ref = MapTypeRef(ListTypeRef(I32), ListTypeRef(I32))
        key = (1,)
        frozen = FrozenMap({key: (2,)})
        first = copy_value(frozen, type_ref, frozen=True)
        second = copy_value(frozen, type_ref, frozen=True)
        assert isinstance(first, dict) and isinstance(second, dict)
        self.assertIs(next(iter(first)), key)
        first_value = first[key]
        assert isinstance(first_value, list)
        first_value.append(3)
        self.assertEqual(second, {key: [2]})
        self.assertEqual(frozen, FrozenMap({key: (2,)}))

    def test_validates_the_requested_representation(self) -> None:
        cases: list[tuple[object, bool]] = [
            ([1, 2], True),  # owned storage in frozen mode
            ((1, 2), False),  # frozen storage in owned mode
            ((1, UNSET), True),
            ((1,), True),
        ]
        for value, frozen in cases:
            with self.subTest(value=value, frozen=frozen):
                with self.assertRaises(EncodeError):
                    copy_value(value, POINT, frozen=frozen)


class ValuesEqualTest(unittest.TestCase):
    def test_owned_and_frozen_storage_compare_alike(self) -> None:
        cases: list[tuple[object, TypeRef]] = [
            ([1, 2], POINT),
            ([], CHOICE),
            ([3, [1, 2]], CHOICE),
            ([[1], []], ListTypeRef(ListTypeRef(I32))),
            ({1: [2]}, MapTypeRef(I32, ListTypeRef(I32))),
            ({(1, 2): None}, SetTypeRef(POINT)),
        ]
        for owned, type_ref in cases:
            with self.subTest(owned=owned):
                frozen = freeze_key(owned, type_ref)
                self.assertTrue(values_equal(owned, frozen))
                self.assertTrue(values_equal(frozen, owned))
                self.assertTrue(values_equal(owned, copy_value(owned, type_ref)))

    def test_detects_differences(self) -> None:
        cases: list[tuple[object, object]] = [
            ([1, 2], (1, 3)),
            ([1, 2], [2, 1]),
            ([], (3, (0, 0))),
            ([[1]], [[1], []]),
            ({1: [2]}, FrozenMap({1: (3,)})),
            ({1: [2]}, {2: [2]}),
            ({1: [2]}, {1: [2], 2: []}),
            ("a", "b"),
        ]
        for left, right in cases:
            with self.subTest(left=left, right=right):
                self.assertFalse(values_equal(left, right))
                self.assertFalse(values_equal(right, left))

    def test_maps_and_sets_compare_without_order(self) -> None:
        self.assertTrue(values_equal({1: None, 2: None}, FrozenMap({2: None, 1: None})))
        self.assertTrue(values_equal({"a": [1], "b": []}, {"b": [], "a": [1]}))

    def test_scalars_compare_by_identity_first(self) -> None:
        nan = math.nan
        self.assertTrue(values_equal([nan], (nan,)))
        self.assertTrue(values_equal({nan: [nan]}, {nan: [nan]}))
        self.assertFalse(values_equal([nan], [float("nan")]))
        self.assertFalse(values_equal({nan: 1}, {float("nan"): 1}))
