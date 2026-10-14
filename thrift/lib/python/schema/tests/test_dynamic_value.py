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

from __future__ import annotations

import copy
import math
import unittest
from typing import cast

import thrift.lib.python.schema._datum as datum
import thrift.lib.python.schema.tests.codec_test.thrift_types as fixtures
from thrift.lib.python.schema.dynamic_value import (
    _read_only_value,
    DynamicValue,
    FrozenMap,
    ListView,
    MapView,
    SetView,
    StructView,
    UnionView,
    UNSET,
)
from thrift.lib.python.schema.tests.datum_testing import (
    ANY,
    any_of_i32,
    BINARY,
    BOOL,
    BYTE,
    containers,
    DOUBLE,
    field_ids,
    FLOAT,
    I16,
    I32,
    I64,
    presence,
    STRING,
    struct_type_of,
    to_value,
    type_ref_of,
)
from thrift.lib.python.schema.type_system import (
    FieldDefinition,
    FieldIdentity,
    InvalidTypeError,
    ListTypeRef,
    MapTypeRef,
    OpaqueAliasNode,
    OpaqueAliasTypeRef,
    PresenceQualifier,
    Primitive,
    PrimitiveTypeRef,
    SetTypeRef,
    StructNode,
    StructTypeRef,
    TypeRef,
    TypeRefBase,
    UnionTypeRef,
)

POINT = struct_type_of(fixtures.Point)
CHOICE = cast(UnionTypeRef, type_ref_of(fixtures.Choice))
WRAPPER = cast(UnionTypeRef, type_ref_of(fixtures.Wrapper))
COLOR = type_ref_of(fixtures.Color)
PRESENCE = struct_type_of(fixtures.Presence)
CONTAINERS = struct_type_of(fixtures.Containers)
FAILURE = struct_type_of(fixtures.Failure)
FIELD_IDS = struct_type_of(fixtures.FieldIds)
EMPTY_UNIONS = struct_type_of(fixtures.EmptyUnions)


def _alias(target: TypeRef, uri: str = "test.dev/dynamic/Alias") -> OpaqueAliasTypeRef:
    return OpaqueAliasTypeRef(OpaqueAliasNode(uri=uri, target_type=target))


def _field_type(struct_type: StructTypeRef, name: str) -> TypeRef:
    field = struct_type.node.field_by_name(name)
    assert field is not None, name
    return field.type


def _value(type_ref: TypeRef, value_datum: object) -> DynamicValue:
    return _read_only_value(type_ref, value_datum)


def _containers() -> DynamicValue:
    return _value(CONTAINERS, to_value(containers(), CONTAINERS))


def _presence() -> DynamicValue:
    return _value(PRESENCE, to_value(presence(), PRESENCE))


def _counts() -> MapView[DynamicValue]:
    return _value(MapTypeRef(STRING, I64), {"a": 1, "b": 2}).as_map()


def _names() -> SetView:
    names = dict.fromkeys(["zeta", "alpha", "mu"])
    return _value(SetTypeRef(STRING), names).as_set()


def _ints(*values: int) -> ListView[DynamicValue]:
    return _value(ListTypeRef(I32), list(values)).as_list()


class ConstructionTest(unittest.TestCase):
    def test_read_only_values_are_not_constructed_directly(self) -> None:
        with self.assertRaises(TypeError):
            DynamicValue(I32, 1)


class ScalarAccessorTest(unittest.TestCase):
    def test_returns_the_stored_value(self) -> None:
        any_value = any_of_i32(1)
        self.assertIs(_value(BOOL, True).as_bool(), True)
        self.assertEqual(_value(BYTE, -1).as_int(), -1)
        self.assertEqual(_value(I16, 2).as_int(), 2)
        self.assertEqual(_value(I32, 3).as_int(), 3)
        self.assertEqual(_value(I64, 2**40).as_int(), 2**40)
        self.assertEqual(_value(COLOR, 99).as_int(), 99)  # open enum
        self.assertEqual(_value(FLOAT, 0.5).as_float(), 0.5)
        self.assertEqual(_value(DOUBLE, 0.1).as_float(), 0.1)
        self.assertEqual(_value(STRING, "s").as_str(), "s")
        self.assertEqual(_value(BINARY, b"b").as_bytes(), b"b")
        self.assertIs(_value(ANY, any_value).as_any(), any_value)

    def test_resolves_opaque_aliases(self) -> None:
        self.assertEqual(_value(_alias(I32), 5).as_int(), 5)
        self.assertEqual(_value(_alias(STRING), "s").as_str(), "s")

    def test_rejects_incompatible_types(self) -> None:
        cases: list[tuple[DynamicValue, str]] = [
            (_value(BOOL, True), "as_int"),
            (_value(I32, 1), "as_bool"),
            (_value(I32, 1), "as_float"),
            (_value(DOUBLE, 1.0), "as_int"),
            (_value(STRING, "s"), "as_bytes"),
            (_value(BINARY, b"b"), "as_str"),
            (_value(COLOR, 1), "as_any"),
            (_value(POINT, [0, 0]), "as_int"),
            (_value(ListTypeRef(I32), []), "as_int"),
        ]
        for value, accessor in cases:
            with self.subTest(value=value, accessor=accessor):
                with self.assertRaises(TypeError):
                    getattr(value, accessor)()


class ExpectTest(unittest.TestCase):
    def test_returns_the_same_value_for_an_equal_reference(self) -> None:
        value = _value(I32, 1)
        self.assertIs(value.expect(PrimitiveTypeRef(Primitive.I32)), value)
        self.assertIs(value.type_ref, I32)

    def test_checks_nominal_identity(self) -> None:
        enum_value = _value(COLOR, 1)
        self.assertIs(enum_value.expect(COLOR), enum_value)
        with self.assertRaises(TypeError):
            enum_value.expect(I32)
        alias = _alias(I32)
        alias_value = _value(alias, 1)
        self.assertIs(alias_value.expect(alias), alias_value)
        with self.assertRaises(TypeError):
            alias_value.expect(I32)
        with self.assertRaises(TypeError):
            _value(I32, 1).expect(I64)


class EqualityTest(unittest.TestCase):
    def test_compares_type_and_value(self) -> None:
        self.assertEqual(_value(I32, 1), _value(I32, 1))
        self.assertNotEqual(_value(I32, 1), _value(I32, 2))
        self.assertNotEqual(_value(I32, 1), _value(I64, 1))
        self.assertNotEqual(_value(I32, 1), _value(COLOR, 1))
        self.assertNotEqual(_value(BOOL, True), _value(I32, 1))
        self.assertNotEqual(_value(_alias(I32), 1), _value(I32, 1))
        self.assertNotEqual(_value(POINT, [1, 2]), _value(POINT, [1, 3]))

    def test_never_equals_a_raw_value(self) -> None:
        self.assertNotEqual(_value(I32, 1), 1)
        self.assertNotEqual(1, _value(I32, 1))
        self.assertNotEqual(_value(POINT, [1, 2]), [1, 2])

    def test_sets_and_maps_compare_without_order(self) -> None:
        set_type = SetTypeRef(I32)
        first = _value(set_type, {1: None, 2: None})
        second = _value(set_type, {2: None, 1: None})
        self.assertEqual(first, second)
        self.assertEqual(list(second.as_set()), [_value(I32, 2), _value(I32, 1)])
        map_type = MapTypeRef(STRING, I32)
        self.assertEqual(
            _value(map_type, {"a": 1, "b": 2}), _value(map_type, {"b": 2, "a": 1})
        )

    def test_a_stored_nan_equals_itself(self) -> None:
        nan = math.nan
        self.assertEqual(_value(DOUBLE, nan), _value(DOUBLE, nan))
        self.assertNotEqual(_value(DOUBLE, float("nan")), _value(DOUBLE, float("nan")))
        self.assertEqual(
            _value(ListTypeRef(DOUBLE), [nan]), _value(ListTypeRef(DOUBLE), [nan])
        )

    def test_owned_and_frozen_storage_compare_by_value(self) -> None:
        labels = _value(MapTypeRef(POINT, STRING), {(1, 2): "a"}).as_map()
        key = next(iter(labels))
        self.assertEqual(key, _value(POINT, [1, 2]))
        self.assertEqual(_value(POINT, [1, 2]), key)
        unions = _value(SetTypeRef(CHOICE), {(): None, (1, 5): None}).as_set()
        self.assertEqual(list(unions), [_value(CHOICE, []), _value(CHOICE, [1, 5])])

    def test_handles_are_unhashable(self) -> None:
        element = next(iter(_value(SetTypeRef(I32), {1: None}).as_set()))
        for value in (_value(I32, 1), element):
            with self.subTest(value=value):
                with self.assertRaises(TypeError):
                    hash(value)


class AttributeAndCopyTest(unittest.TestCase):
    def test_rejects_attribute_assignment(self) -> None:
        value = _value(I32, 1)
        for name in ("type_ref", "_datum", "_type_ref", "_frozen"):
            with self.subTest(name=name):
                with self.assertRaises(AttributeError):
                    setattr(value, name, 2)
                with self.assertRaises(AttributeError):
                    delattr(value, name)
        self.assertEqual(value, _value(I32, 1))

    def test_copy_module_keeps_the_handle_kind(self) -> None:
        value = _value(POINT, [1, 2])
        for copied in (copy.copy(value), copy.deepcopy(value)):
            with self.subTest(copied=copied):
                self.assertIs(type(copied), DynamicValue)
                self.assertEqual(copied, value)


class ReprAndExportTest(unittest.TestCase):
    def test_repr_names_the_class_and_type(self) -> None:
        self.assertEqual(repr(_value(I32, 5)), "DynamicValue(i32, 5)")
        self.assertEqual(
            repr(_value(ListTypeRef(STRING), ["a"])),
            "DynamicValue(list<string>, ['a'])",
        )
        self.assertEqual(
            repr(_value(POINT, [1, 2])),
            f"DynamicValue({POINT.node.uri}, [1, 2])",
        )

    def test_to_datum_returns_an_owned_copy(self) -> None:
        value = _value(MapTypeRef(I32, ListTypeRef(I32)), {1: [2]})
        exported = cast(dict[int, list[int]], value.to_datum())
        self.assertEqual(exported, {1: [2]})
        exported[1].append(3)
        self.assertEqual(value.to_datum(), {1: [2]})
        key = next(iter(_value(MapTypeRef(POINT, I32), {(1, 2): 3}).as_map()))
        self.assertEqual(key.to_datum(), [1, 2])

    def test_reexports_the_datum_representation(self) -> None:
        self.assertIs(FrozenMap, datum.FrozenMap)
        self.assertIs(UNSET, datum.UNSET)


class ChildTypeTest(unittest.TestCase):
    def test_struct_fields_use_the_field_type(self) -> None:
        view = _containers().as_struct()
        for field in CONTAINERS.node.fields:
            with self.subTest(field=field.identity.name):
                self.assertIs(view[field.identity.name].type_ref, field.type)

    def test_container_children_use_the_declared_types(self) -> None:
        view = _containers().as_struct()
        ints_type = cast(ListTypeRef, _field_type(CONTAINERS, "ints"))
        self.assertIs(view["ints"].as_list()[0].type_ref, ints_type.element_type)
        names_type = cast(SetTypeRef, _field_type(CONTAINERS, "names"))
        for name in view["names"].as_set():
            self.assertIs(name.type_ref, names_type.element_type)
        counts_type = cast(MapTypeRef, _field_type(CONTAINERS, "counts"))
        for key, value in view["counts"].as_map().items():
            self.assertIs(key.type_ref, counts_type.key_type)
            self.assertIs(value.type_ref, counts_type.value_type)

    def test_nested_children_keep_enum_identity_and_widths(self) -> None:
        view = _containers().as_struct()
        for key in view["weights"].as_map():
            self.assertIsInstance(key.expect(COLOR).as_int(), int)
        deep_type = cast(ListTypeRef, _field_type(CONTAINERS, "deep"))
        inner_type = cast(MapTypeRef, deep_type.element_type)
        set_type = cast(SetTypeRef, inner_type.value_type)
        inner = view["deep"].as_list()[0].as_map().lookup("k")
        self.assertIs(inner.type_ref, set_type)
        for element in inner.as_set():
            self.assertIs(element.type_ref, set_type.element_type)
            element.expect(I16)

    def test_alias_of_a_container_uses_the_container_child_type(self) -> None:
        element = PrimitiveTypeRef(Primitive.I16)
        listed = _value(_alias(ListTypeRef(element)), [1, 2])
        self.assertIs(listed.as_list()[0].type_ref, element)
        in_set = _value(_alias(SetTypeRef(element)), {1: None})
        self.assertIs(next(iter(in_set.as_set())).type_ref, element)
        mapped = _value(_alias(MapTypeRef(element, STRING)), {1: "a"})
        (key, value), *_ = mapped.as_map().items()
        self.assertIs(key.type_ref, element)
        self.assertIs(value.type_ref, STRING)

    def test_union_fields_use_the_field_type(self) -> None:
        child = _value(CHOICE, [3, [1, 2]]).as_union().field("point")
        point_field = CHOICE.node.field_by_name("point")
        assert point_field is not None
        self.assertIs(child.type_ref, point_field.type)

    def test_container_accessors_reject_other_types(self) -> None:
        cases: list[tuple[DynamicValue, str]] = [
            (_value(I32, 1), "as_list"),
            (_value(I32, 1), "as_set"),
            (_value(I32, 1), "as_map"),
            (_value(I32, 1), "as_struct"),
            (_value(I32, 1), "as_union"),
            (_value(ListTypeRef(I32), []), "as_set"),
            (_value(CHOICE, []), "as_struct"),
            (_value(POINT, [0, 0]), "as_union"),
        ]
        for value, accessor in cases:
            with self.subTest(value=value, accessor=accessor):
                with self.assertRaises(TypeError):
                    getattr(value, accessor)()


class StructViewTest(unittest.TestCase):
    def test_absent_optional_fields_are_omitted(self) -> None:
        view = _value(PRESENCE, datum.default_value(PRESENCE)).as_struct()
        self.assertIsInstance(view, StructView)
        self.assertNotIn("maybe", view)
        with self.assertRaises(KeyError):
            view["maybe"]
        self.assertIsNone(view.get("maybe"))
        expected = [
            f.identity.name
            for f in PRESENCE.node.fields
            if f.presence is not PresenceQualifier.OPTIONAL
        ]
        self.assertEqual(list(view), expected)
        self.assertEqual(len(view), len(expected))

    def test_non_optional_fields_hold_explicit_defaults(self) -> None:
        view = _value(PRESENCE, datum.default_value(PRESENCE)).as_struct()
        self.assertEqual(view["plain"], _value(I32, 0))
        self.assertEqual(view["terse"], _value(I32, 0))
        self.assertEqual(view["plain_default"], _value(I32, 7))
        self.assertEqual(view["color_default"].expect(COLOR).as_int(), 2)
        choice = view["choice_default"].as_union()
        self.assertEqual(choice.field("text").as_str(), "hi")

    def test_set_optional_fields_are_present(self) -> None:
        view = _presence().as_struct()
        self.assertEqual(view["maybe"].as_int(), 2)
        self.assertEqual(view.by_id(2).as_int(), 2)
        self.assertEqual(len(view), len(PRESENCE.node.fields))

    def test_field_ids_are_not_list_indexes(self) -> None:
        view = _value(FIELD_IDS, to_value(field_ids(), FIELD_IDS)).as_struct()
        self.assertEqual(
            list(view), ["one", "seventeen", "two", "negative", "far_bool", "max_id"]
        )
        self.assertEqual(view.by_id(2).as_int(), 2)
        self.assertEqual(view.by_id(17).as_int(), 17)
        self.assertEqual(view.by_id(-5).as_int(), -5)
        self.assertIs(view.by_id(300).as_bool(), True)
        self.assertEqual(view.by_id(32767).as_int(), 2**40)

    def test_rejects_bool_and_non_int_field_ids(self) -> None:
        view = _value(FIELD_IDS, to_value(field_ids(), FIELD_IDS)).as_struct()
        with self.assertRaises(TypeError):
            view.by_id(True)  # field 1 exists
        with self.assertRaises(TypeError):
            view.by_id(cast(int, "one"))

    def test_unknown_and_absent_fields(self) -> None:
        view = _value(PRESENCE, datum.default_value(PRESENCE)).as_struct()
        with self.assertRaises(KeyError):
            view["nope"]
        self.assertIsNone(view.get("nope"))
        self.assertNotIn("nope", view)
        with self.assertRaises(KeyError):
            view.by_id(99)
        with self.assertRaises(KeyError):
            view.by_id(2)  # optional and unset

    def test_exceptions_are_structs(self) -> None:
        view = _value(FAILURE, ["why", 500]).as_struct()
        self.assertEqual(view["reason"].as_str(), "why")
        self.assertEqual(view["code"].as_int(), 500)

    def test_explicit_empty_union_survives_a_custom_default(self) -> None:
        default = datum.default_value(PRESENCE)
        assert isinstance(default, list)
        names = [f.identity.name for f in PRESENCE.node.fields]
        index = names.index("choice_default")
        self.assertEqual(default[index], [2, "hi"])
        default[index] = []
        child = _value(PRESENCE, default).as_struct()["choice_default"]
        self.assertEqual(child.to_datum(), [])
        self.assertIs(child.type_ref, _field_type(PRESENCE, "choice_default"))
        self.assertIsNone(child.as_union().active)


class UnionViewTest(unittest.TestCase):
    def test_active_field(self) -> None:
        view = _value(CHOICE, [1, 5]).as_union()
        self.assertIsInstance(view, UnionView)
        active = view.active
        assert active is not None
        field, value = active
        self.assertIs(field, CHOICE.node.field_by_id(1))
        self.assertIs(value.type_ref, field.type)
        self.assertEqual(value, _value(I32, 5))
        self.assertEqual(view.field("number"), value)
        self.assertEqual(view.field(1), value)
        self.assertEqual(view.get("number"), value)

    def test_inactive_and_unknown_fields(self) -> None:
        view = _value(CHOICE, [1, 5]).as_union()
        for selector in ("text", 2, "nope", 99):
            with self.subTest(selector=selector):
                with self.assertRaises(KeyError):
                    view.field(selector)
                self.assertIsNone(view.get(selector))
        with self.assertRaises(TypeError):
            view.field(True)
        with self.assertRaises(TypeError):
            view.get(True)

    def test_empty_union(self) -> None:
        view = _value(CHOICE, []).as_union()
        self.assertIsNone(view.active)
        with self.assertRaises(KeyError):
            view.field("number")
        self.assertIsNone(view.get("number"))

    def test_empty_union_children_are_present(self) -> None:
        obj = fixtures.EmptyUnions(
            maybe_choice=fixtures.Choice(),
            wrapper=fixtures.Wrapper(choice=fixtures.Choice()),
        )
        view = _value(EMPTY_UNIONS, to_value(obj, EMPTY_UNIONS)).as_struct()
        maybe = view["maybe_choice"]
        self.assertEqual(maybe.to_datum(), [])
        self.assertIs(maybe.type_ref, _field_type(EMPTY_UNIONS, "maybe_choice"))
        inner = view["wrapper"].as_union().field("choice")
        self.assertEqual(inner.to_datum(), [])
        self.assertEqual(inner.type_ref, CHOICE)
        default = _value(EMPTY_UNIONS, datum.default_value(EMPTY_UNIONS))
        self.assertNotIn("maybe_choice", default.as_struct())

    def test_equality_compares_union_type_and_value(self) -> None:
        view = _value(CHOICE, [1, 5]).as_union()
        self.assertEqual(view, _value(CHOICE, [1, 5]).as_union())
        self.assertNotEqual(view, _value(CHOICE, [1, 6]).as_union())
        self.assertNotEqual(view, _value(CHOICE, [2, "5"]).as_union())
        self.assertNotEqual(view, _value(CHOICE, []).as_union())
        empty = _value(CHOICE, []).as_union()
        self.assertEqual(empty, _value(CHOICE, []).as_union())
        self.assertNotEqual(empty, _value(WRAPPER, []).as_union())
        for other in [[1, 5], _value(CHOICE, [1, 5])]:
            with self.subTest(other=other):
                self.assertNotEqual(view, other)
                self.assertNotEqual(other, view)

    def test_is_unhashable(self) -> None:
        with self.assertRaises(TypeError):
            hash(_value(CHOICE, [1, 5]).as_union())


class ListViewTest(unittest.TestCase):
    def test_sequence_protocol(self) -> None:
        items = _value(ListTypeRef(I32), [10, 20, 30]).as_list()
        self.assertIsInstance(items, ListView)
        self.assertEqual(len(items), 3)
        self.assertEqual(items[0], _value(I32, 10))
        self.assertEqual(items[-1], _value(I32, 30))
        with self.assertRaises(IndexError):
            items[3]
        self.assertEqual(list(items), [_value(I32, v) for v in (10, 20, 30)])
        self.assertEqual(list(reversed(items))[0], _value(I32, 30))
        self.assertEqual(items.index(_value(I32, 30)), 2)

    def test_slicing_returns_a_tuple_of_values(self) -> None:
        items = _ints(10, 20, 30)
        self.assertEqual(items[1:], (_value(I32, 20), _value(I32, 30)))
        self.assertEqual(items[::-2], (_value(I32, 30), _value(I32, 10)))

    def test_membership_is_typed(self) -> None:
        items = _ints(10, 20)
        self.assertIn(_value(I32, 20), items)
        self.assertNotIn(20, items)
        self.assertNotIn(_value(I64, 20), items)

    def test_equality_compares_element_type_and_elements(self) -> None:
        list_type = ListTypeRef(I32)
        items = _value(list_type, [10, 20]).as_list()
        self.assertEqual(items, _value(list_type, [10, 20]).as_list())
        self.assertEqual(items, _value(_alias(list_type), [10, 20]).as_list())
        self.assertNotEqual(items, _value(list_type, [20, 10]).as_list())
        self.assertNotEqual(items, _value(list_type, [10]).as_list())
        self.assertNotEqual(items, _value(ListTypeRef(I64), [10, 20]).as_list())
        typed = (_value(I32, 10), _value(I32, 20))
        for other in [typed, list(typed), (10, 20)]:
            with self.subTest(other=other):
                self.assertNotEqual(items, other)
                self.assertNotEqual(other, items)

    def test_a_stored_nan_element_equals_itself(self) -> None:
        list_type = ListTypeRef(DOUBLE)
        nan = math.nan
        self.assertEqual(
            _value(list_type, [nan]).as_list(), _value(list_type, [nan]).as_list()
        )
        self.assertNotEqual(
            _value(list_type, [float("nan")]).as_list(),
            _value(list_type, [float("nan")]).as_list(),
        )

    def test_is_unhashable(self) -> None:
        with self.assertRaises(TypeError):
            hash(_ints())


class MapViewTest(unittest.TestCase):
    def test_typed_indexing_and_get(self) -> None:
        counts = _counts()
        a = _value(STRING, "a")
        self.assertEqual(counts[a], _value(I64, 1))
        self.assertEqual(counts.get(a), _value(I64, 1))
        missing = _value(STRING, "zz")
        with self.assertRaises(KeyError):
            counts[missing]
        self.assertIsNone(counts.get(missing))
        self.assertEqual(counts.get(missing, a), a)

    def test_a_key_with_another_type_is_missing(self) -> None:
        counts = _counts()
        other = _value(_alias(STRING), "a")
        with self.assertRaises(KeyError):
            counts[other]
        self.assertIsNone(counts.get(other))
        self.assertNotIn(other, counts)
        self.assertNotIn(other, counts.keys())

    def test_raw_keys_are_rejected_by_indexing_and_get(self) -> None:
        counts = _counts()
        with self.assertRaises(TypeError):
            counts[cast(DynamicValue, "a")]
        with self.assertRaises(TypeError):
            counts.get(cast(DynamicValue, "a"))

    def test_raw_membership_is_false(self) -> None:
        counts = _counts()
        a, one = _value(STRING, "a"), _value(I64, 1)
        self.assertNotIn("a", counts)
        self.assertNotIn("a", counts.keys())
        for item in [("a", 1), (a, 1), ("a", one), "a", (a, one, one), [a, one]]:
            with self.subTest(item=item):
                self.assertNotIn(item, counts.items())

    def test_typed_membership_agrees_with_indexing_and_iteration(self) -> None:
        counts = _counts()
        for key in counts:
            self.assertIn(key, counts)
            self.assertIn(key, counts.keys())
            self.assertIn((key, counts[key]), counts.items())
        a = _value(STRING, "a")
        self.assertIn((a, _value(I64, 1)), counts.items())
        self.assertNotIn((a, _value(I64, 2)), counts.items())
        self.assertNotIn((a, _value(I32, 1)), counts.items())
        self.assertNotIn(_value(STRING, "zz"), counts)

    def test_iteration_is_typed_and_ordered(self) -> None:
        counts = _counts()
        keys = [_value(STRING, "a"), _value(STRING, "b")]
        values = [_value(I64, 1), _value(I64, 2)]
        self.assertEqual(len(counts), 2)
        self.assertEqual(list(counts), keys)
        self.assertEqual(list(counts.keys()), keys)
        self.assertEqual(list(counts.values()), values)
        self.assertEqual(list(counts.items()), list(zip(keys, values)))
        self.assertIn(_value(I64, 2), counts.values())
        self.assertNotIn(2, counts.values())

    def test_keys_and_items_compare_as_typed_sets(self) -> None:
        counts = _counts()
        self.assertEqual(counts.keys(), _counts().keys())
        self.assertEqual(counts.items(), _counts().items())
        self.assertNotEqual(counts.keys(), {"a", "b"})
        self.assertNotEqual(counts.items(), {("a", 1), ("b", 2)})

    def test_equality_compares_types_and_entries(self) -> None:
        map_type = MapTypeRef(STRING, I64)
        counts = _counts()
        self.assertEqual(counts, _value(map_type, {"b": 2, "a": 1}).as_map())
        self.assertNotEqual(counts, _value(map_type, {"a": 1}).as_map())
        self.assertNotEqual(
            counts, _value(MapTypeRef(STRING, I32), {"a": 1, "b": 2}).as_map()
        )
        self.assertNotEqual(counts, {"a": 1, "b": 2})

    def test_lookup_accepts_raw_and_typed_probes(self) -> None:
        counts = _counts()
        self.assertEqual(counts.lookup("a"), _value(I64, 1))
        self.assertEqual(counts.lookup(_value(STRING, "b")), _value(I64, 2))
        with self.assertRaises(KeyError):
            counts.lookup("zz")
        for probe in [1, b"a", None, _value(_alias(STRING), "a")]:
            with self.subTest(probe=probe):
                with self.assertRaises(TypeError):
                    counts.lookup(probe)
                with self.assertRaises(TypeError):
                    counts.find_key(probe)

    def test_find_key_returns_the_stored_key(self) -> None:
        key_type = SetTypeRef(I32)
        view = _value(
            MapTypeRef(key_type, STRING), {FrozenMap({1: None, 2: None}): "x"}
        )
        found = view.as_map().find_key({2: None, 1: None})
        assert found is not None
        self.assertEqual(found.to_datum(), {1: None, 2: None})
        self.assertIs(found.type_ref, key_type)
        self.assertIsNone(view.as_map().find_key({3: None}))
        for probe in [frozenset({1, 2}), FrozenMap({1: None, 2: None})]:
            with self.subTest(probe=probe):
                with self.assertRaises(TypeError):
                    view.as_map().find_key(probe)

    def test_struct_keys(self) -> None:
        labels = _value(MapTypeRef(POINT, STRING), {(0, 0): "origin"}).as_map()
        self.assertEqual(labels.lookup([0, 0]), _value(STRING, "origin"))
        self.assertEqual(labels.lookup(_value(POINT, [0, 0])), _value(STRING, "origin"))
        with self.assertRaises(KeyError):
            labels.lookup([1, 1])
        for probe in [[UNSET, 0], [0], (0, 0)]:
            with self.subTest(probe=probe):
                with self.assertRaises(TypeError):
                    labels.lookup(probe)

    def test_bool_and_int_keys_do_not_collide(self) -> None:
        by_int = _value(MapTypeRef(I32, STRING), {1: "one"}).as_map()
        with self.assertRaises(TypeError):
            by_int.lookup(True)
        with self.assertRaises(TypeError):
            by_int.find_key(True)
        self.assertNotIn(_value(BOOL, True), by_int)
        with self.assertRaises(KeyError):
            by_int[_value(BOOL, True)]
        by_bool = _value(MapTypeRef(BOOL, STRING), {True: "y"}).as_map()
        with self.assertRaises(TypeError):
            by_bool.lookup(1)
        self.assertEqual(by_bool.lookup(True), _value(STRING, "y"))

    def test_nan_keys_match_by_identity(self) -> None:
        nan = float("nan")
        view = _value(MapTypeRef(DOUBLE, STRING), {nan: "x"}).as_map()
        found = view.find_key(nan)
        assert found is not None
        self.assertIs(found.as_float(), nan)
        self.assertIsNone(view.find_key(float("nan")))
        key = next(iter(view))
        self.assertIn(key, view)
        self.assertEqual(view[key], _value(STRING, "x"))

    def test_malformed_schema_errors_propagate(self) -> None:
        # Only validating a probe reaches the unresolved field type.
        broken = StructTypeRef(
            StructNode(
                uri="test.dev/dynamic/Broken",
                fields=[
                    FieldDefinition(
                        identity=FieldIdentity(1, "items"),
                        presence=PresenceQualifier.UNQUALIFIED,
                        type=ListTypeRef(TypeRefBase()),
                    )
                ],
            )
        )
        view = _value(MapTypeRef(broken, I32), {}).as_map()
        with self.assertRaises(InvalidTypeError):
            view.lookup([[]])
        with self.assertRaises(InvalidTypeError):
            view.find_key([[]])
        elements = _value(SetTypeRef(broken), {}).as_set()
        with self.assertRaises(InvalidTypeError):
            elements.find([[]])


class SetViewTest(unittest.TestCase):
    def test_typed_membership_and_order(self) -> None:
        names = _names()
        self.assertIsInstance(names, SetView)
        self.assertEqual(
            list(names), [_value(STRING, n) for n in ("zeta", "alpha", "mu")]
        )
        self.assertEqual(len(names), 3)
        self.assertIn(_value(STRING, "alpha"), names)
        for probe in ["alpha", _value(_alias(STRING), "alpha")]:
            with self.subTest(probe=probe):
                self.assertNotIn(probe, names)
        for element in names:
            self.assertIn(element, names)

    def test_find_accepts_raw_and_typed_probes(self) -> None:
        names = _names()
        self.assertEqual(names.find("alpha"), _value(STRING, "alpha"))
        self.assertEqual(names.find(_value(STRING, "mu")), _value(STRING, "mu"))
        self.assertIsNone(names.find("nope"))
        for probe in [b"alpha", 1, _value(BINARY, b"alpha")]:
            with self.subTest(probe=probe):
                with self.assertRaises(TypeError):
                    names.find(probe)

    def test_struct_elements(self) -> None:
        points = _value(SetTypeRef(POINT), {(1, 2): None}).as_set()
        self.assertIn(_value(POINT, [1, 2]), points)
        self.assertEqual(points.find([1, 2]), _value(POINT, [1, 2]))
        self.assertIsNone(points.find([2, 1]))

    def test_bool_and_int_elements_do_not_collide(self) -> None:
        ints = _value(SetTypeRef(I64), {1: None, 0: None}).as_set()
        with self.assertRaises(TypeError):
            ints.find(True)
        self.assertNotIn(_value(BOOL, True), ints)

    def test_nan_representatives(self) -> None:
        nan = float("nan")
        view = _value(SetTypeRef(DOUBLE), {nan: None, 1.0: None}).as_set()
        first, second = next(iter(view)), next(iter(view))
        self.assertEqual(first, second)
        self.assertIn(first, view)
        found = view.find(nan)
        assert found is not None
        self.assertIs(found.as_float(), nan)
        self.assertIsNone(view.find(float("nan")))

    def test_equality_ignores_order_and_checks_element_types(self) -> None:
        names = _names()
        reordered = _value(SetTypeRef(STRING), dict.fromkeys(["mu", "zeta", "alpha"]))
        self.assertEqual(names, reordered.as_set())
        aliased = _value(
            SetTypeRef(_alias(STRING)), dict.fromkeys(["zeta", "alpha", "mu"])
        )
        self.assertNotEqual(names, aliased.as_set())
        self.assertNotEqual(names, {"zeta", "alpha", "mu"})
        smaller = _value(SetTypeRef(STRING), dict.fromkeys(["mu"])).as_set()
        self.assertTrue(smaller <= names and smaller < names)
        self.assertFalse(names <= smaller)


class ReadOnlyTest(unittest.TestCase):
    def test_frozen_unions_and_nested_containers_are_readable(self) -> None:
        key_type = MapTypeRef(I32, ListTypeRef(CHOICE))
        nested = _value(SetTypeRef(key_type), {FrozenMap({1: ((3, (1, 2)), ())}): None})
        (element,) = nested.as_set()
        choices = element.as_map().lookup(1).as_list()
        self.assertEqual(choices[0].as_union().field("point"), _value(POINT, [1, 2]))
        self.assertIsNone(choices[1].as_union().active)
        self.assertIs(type(choices[0]), DynamicValue)
