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
import gc
import math
import unittest
from typing import cast

import thrift.lib.python.schema._datum as datum
import thrift.lib.python.schema.tests.codec_test.thrift_types as fixtures
from apache.thrift.type.any_rep.thrift_types import AnyStruct
from thrift.lib.python.schema._errors import EncodeError
from thrift.lib.python.schema.dynamic_value import (
    DynamicValue,
    FrozenMap,
    ListView,
    MapView,
    MutableDynamicValue,
    MutableListView,
    MutableMapView,
    MutableSetView,
    MutableStructView,
    MutableUnionView,
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
    sample_objects,
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
NESTED = struct_type_of(fixtures.Nested)

MUTABLE_VIEWS = (
    MutableStructView,
    MutableListView,
    MutableMapView,
    MutableSetView,
    MutableUnionView,
)


def _alias(target: TypeRef, uri: str = "test.dev/dynamic/Alias") -> OpaqueAliasTypeRef:
    return OpaqueAliasTypeRef(OpaqueAliasNode(uri=uri, target_type=target))


def _field_type(struct_type: StructTypeRef, name: str) -> TypeRef:
    field = struct_type.node.field_by_name(name)
    assert field is not None, name
    return field.type


def _value(type_ref: TypeRef, value_datum: object) -> MutableDynamicValue:
    return MutableDynamicValue(type_ref, value_datum)


def _containers() -> MutableDynamicValue:
    return _value(CONTAINERS, to_value(containers(), CONTAINERS))


def _presence() -> MutableDynamicValue:
    return _value(PRESENCE, to_value(presence(), PRESENCE))


def _counts() -> MapView[DynamicValue]:
    return _value(MapTypeRef(STRING, I64), {"a": 1, "b": 2}).read_only().as_map()


def _names() -> SetView:
    names = dict.fromkeys(["zeta", "alpha", "mu"])
    return _value(SetTypeRef(STRING), names).read_only().as_set()


def _ints(*values: int) -> MutableListView:
    return _value(ListTypeRef(I32), list(values)).as_list()


class ConstructionTest(unittest.TestCase):
    def test_keeps_the_declared_reference_and_copies_the_datum(self) -> None:
        for name, obj in sample_objects():
            with self.subTest(name=name):
                type_ref = type_ref_of(type(obj))
                value_datum = to_value(obj, type_ref)
                value = _value(type_ref, value_datum)
                self.assertIs(value.type_ref, type_ref)
                self.assertEqual(value.to_datum(), value_datum)
                self.assertIsNot(value.to_datum(), value_datum)

    def test_isolates_caller_storage(self) -> None:
        raw = [1, 2]
        point = _value(POINT, raw)
        raw[0] = 99
        self.assertEqual(point.as_struct()["x"].as_int(), 1)
        exported = point.to_datum()
        assert isinstance(exported, list)
        exported[1] = 99
        self.assertEqual(point.as_struct()["y"].as_int(), 2)

    def test_repeated_input_branches_become_independent(self) -> None:
        shared = [1, 2]
        points = _value(ListTypeRef(POINT), [shared, shared]).as_list()
        points[0].as_struct()["x"] = 5
        self.assertEqual(points[1].as_struct()["x"].as_int(), 1)

    def test_rejects_datum_that_does_not_fit(self) -> None:
        cases: list[tuple[TypeRef, object]] = [
            (I32, True),
            (BOOL, 1),
            (DOUBLE, 1),  # no implicit conversion
            (FLOAT, 0.1),  # not exactly representable in float32
            (BYTE, 128),
            (STRING, "\ud800"),
            (ListTypeRef(I32), (1, 2)),  # frozen storage
            (SetTypeRef(I32), frozenset({1})),
            (SetTypeRef(I32), {1: 2}),  # set payloads are None
            (MapTypeRef(I32, I32), FrozenMap({1: 2})),  # frozen storage
            (POINT, [1]),
            (POINT, (1, 2)),
            (CHOICE, [99, 1]),
            (ANY, None),
        ]
        for type_ref, value_datum in cases:
            with self.subTest(type_ref=type_ref, datum=value_datum):
                with self.assertRaises(EncodeError):
                    _value(type_ref, value_datum)

    def test_rejects_cyclic_datum(self) -> None:
        cyclic = datum.default_value(NESTED)
        assert isinstance(cyclic, list)
        names = [f.identity.name for f in NESTED.node.fields]
        cyclic[names.index("next")] = cyclic
        with self.assertRaisesRegex(EncodeError, "cyclic"):
            _value(NESTED, cyclic)

    def test_rejects_unset_in_non_optional_fields(self) -> None:
        cases: list[tuple[TypeRef, object]] = [
            (POINT, [UNSET, 0]),
            (ListTypeRef(POINT), [[0, UNSET]]),
            (SetTypeRef(POINT), {(UNSET, 0): None}),
            (MapTypeRef(POINT, I32), {(UNSET, 0): 1}),
            (CHOICE, [3, [0, UNSET]]),
            (_alias(ListTypeRef(POINT)), [[UNSET, UNSET]]),
        ]
        for type_ref, value_datum in cases:
            with self.subTest(type_ref=type_ref, datum=value_datum):
                with self.assertRaisesRegex(EncodeError, r"Point\.[xy]"):
                    _value(type_ref, value_datum)

    def test_accepts_unset_in_optional_fields(self) -> None:
        default = datum.default_value(PRESENCE)
        assert isinstance(default, list)
        self.assertIn(UNSET, default)
        self.assertEqual(_value(PRESENCE, default).to_datum(), default)

    def test_rejects_unresolved_schemas(self) -> None:
        with self.assertRaises(InvalidTypeError):
            _value(cast(TypeRef, TypeRefBase()), 1)
        with self.assertRaises(InvalidTypeError):
            _value(ListTypeRef(TypeRefBase()), [])

    def test_read_only_values_are_not_constructed_directly(self) -> None:
        with self.assertRaises(TypeError):
            DynamicValue(I32, 1)


class FactoryTest(unittest.TestCase):
    def test_default_materializes_schema_defaults(self) -> None:
        self.assertEqual(MutableDynamicValue.default(I32), _value(I32, 0))
        self.assertEqual(MutableDynamicValue.default(CHOICE).to_datum(), [])
        for cls in (fixtures.Primitives, fixtures.Containers, fixtures.Presence):
            with self.subTest(cls=cls.__name__):
                type_ref = type_ref_of(cls)
                value = MutableDynamicValue.default(type_ref)
                self.assertIsInstance(value, MutableDynamicValue)
                self.assertIs(value.type_ref, type_ref)
                self.assertEqual(value.to_datum(), to_value(cls(), type_ref))

    def test_defaults_are_independent(self) -> None:
        first = MutableDynamicValue.default(PRESENCE)
        second = MutableDynamicValue.default(PRESENCE)
        first.as_struct()["ints_default"].as_list().append(4)
        first.as_struct()["point_default"].as_struct()["x"] = 9
        self.assertEqual(second, MutableDynamicValue.default(PRESENCE))
        self.assertNotEqual(first, second)

    def test_default_of_an_alias_keeps_the_alias(self) -> None:
        alias = _alias(ListTypeRef(I32))
        value = MutableDynamicValue.default(alias)
        self.assertIs(value.type_ref, alias)
        self.assertEqual(value.to_datum(), [])


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
        self.assertIsInstance(MutableDynamicValue.default(ANY).as_any(), AnyStruct)
        self.assertEqual(_value(I32, 3).read_only().as_int(), 3)

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

    def test_mutable_and_read_only_handles_compare_by_value(self) -> None:
        value = _value(POINT, [1, 2])
        self.assertEqual(value, value.read_only())
        self.assertEqual(value.read_only(), value)

    def test_owned_and_frozen_storage_compare_by_value(self) -> None:
        labels = _value(MapTypeRef(POINT, STRING), {(1, 2): "a"}).as_map()
        key = next(iter(labels))
        self.assertEqual(key, _value(POINT, [1, 2]))
        self.assertEqual(_value(POINT, [1, 2]), key)
        unions = _value(SetTypeRef(CHOICE), {(): None, (1, 5): None}).as_set()
        self.assertEqual(list(unions), [_value(CHOICE, []), _value(CHOICE, [1, 5])])

    def test_handles_are_unhashable(self) -> None:
        element = next(iter(_value(SetTypeRef(I32), {1: None}).as_set()))
        for value in (_value(I32, 1), _value(I32, 1).read_only(), element):
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

    def test_copy_returns_an_independent_mutable_value(self) -> None:
        value = _value(POINT, [1, 2])
        for copied in (value.copy(), value.read_only().copy()):
            with self.subTest(copied=copied):
                self.assertIs(type(copied), MutableDynamicValue)
                self.assertEqual(copied, value)
                copied.as_struct()["x"] = 9
                self.assertEqual(value.as_struct()["x"].as_int(), 1)

    def test_copy_module_keeps_the_handle_kind_and_isolates_storage(self) -> None:
        value = _value(POINT, [1, 2])
        for original in (value, value.read_only()):
            for copied in (copy.copy(original), copy.deepcopy(original)):
                with self.subTest(original=original, copied=copied):
                    self.assertIs(type(copied), type(original))
                    self.assertEqual(copied, original)
        shallow = copy.copy(value)
        shallow.as_struct()["x"] = 9
        self.assertEqual(value.as_struct()["x"].as_int(), 1)


class ReprAndExportTest(unittest.TestCase):
    def test_repr_names_the_class_and_type(self) -> None:
        self.assertEqual(repr(_value(I32, 5)), "MutableDynamicValue(i32, 5)")
        self.assertEqual(
            repr(_value(ListTypeRef(STRING), ["a"])),
            "MutableDynamicValue(list<string>, ['a'])",
        )
        self.assertEqual(
            repr(_value(POINT, [1, 2]).read_only()),
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
        view = MutableDynamicValue.default(PRESENCE).read_only().as_struct()
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
        view = MutableDynamicValue.default(PRESENCE).as_struct()
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
        view = MutableDynamicValue.default(PRESENCE).as_struct()
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
        view = _value(CHOICE, [1, 5]).read_only().as_union()
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
        default = MutableDynamicValue.default(EMPTY_UNIONS)
        self.assertNotIn("maybe_choice", default.as_struct())

    def test_equality_compares_union_type_and_value(self) -> None:
        view = _value(CHOICE, [1, 5]).as_union()
        self.assertEqual(view, _value(CHOICE, [1, 5]).read_only().as_union())
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
        items = _value(ListTypeRef(I32), [10, 20, 30]).read_only().as_list()
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
        self.assertEqual(items, _value(list_type, [10, 20]).read_only().as_list())
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


class MutableStructViewTest(unittest.TestCase):
    def test_mutable_values_return_mutable_views(self) -> None:
        fields = MutableDynamicValue.default(PRESENCE).as_struct()
        self.assertIsInstance(fields, MutableStructView)
        self.assertIsInstance(fields["point_default"], MutableDynamicValue)
        self.assertIsInstance(fields["point_default"].as_struct(), MutableStructView)
        self.assertIsInstance(fields["ints_default"].as_list(), MutableListView)
        self.assertIsInstance(fields["map_default"].as_map(), MutableMapView)
        self.assertIsInstance(fields["set_default"].as_set(), MutableSetView)
        self.assertIsInstance(fields["choice_default"].as_union(), MutableUnionView)

    def test_item_assignment_accepts_raw_and_typed_values(self) -> None:
        fields = MutableDynamicValue.default(PRESENCE).as_struct()
        fields["plain"] = 5
        self.assertEqual(fields["plain"], _value(I32, 5))
        fields["plain"] = _value(I32, 6).read_only()
        self.assertEqual(fields["plain"].as_int(), 6)
        fields["maybe"] = 1
        self.assertIn("maybe", fields)
        fields["point_default"] = [3, 4]
        self.assertEqual(fields["point_default"], _value(POINT, [3, 4]))
        fields["color_default"] = _value(COLOR, 1)
        self.assertEqual(fields["color_default"].expect(COLOR).as_int(), 1)

    def test_set_field_selects_by_name_or_id(self) -> None:
        fields = MutableDynamicValue.default(PRESENCE).as_struct()
        fields.set_field("plain", 1)
        fields.set_field(4, 2)  # plain_default
        self.assertEqual(fields["plain"].as_int(), 1)
        self.assertEqual(fields["plain_default"].as_int(), 2)
        for selector in ("nope", 99):
            with self.subTest(selector=selector):
                with self.assertRaises(KeyError):
                    fields.set_field(selector, 1)
        with self.assertRaises(KeyError):
            fields["nope"] = 1
        with self.assertRaises(TypeError):
            fields.set_field(True, 1)

    def test_typed_values_keep_declared_identity(self) -> None:
        value = MutableDynamicValue.default(PRESENCE)
        fields = value.as_struct()
        cases: list[tuple[str, DynamicValue]] = [
            ("color_default", _value(I32, 1)),
            ("plain", _value(COLOR, 1)),
            ("plain", _value(_alias(I32), 1)),
            ("plain", _value(I64, 1)),
        ]
        for name, typed in cases:
            with self.subTest(name=name, typed=typed):
                with self.assertRaises(TypeError):
                    fields[name] = typed
        self.assertEqual(value, MutableDynamicValue.default(PRESENCE))

    def test_raw_values_are_validated_without_coercion(self) -> None:
        value = MutableDynamicValue.default(PRESENCE)
        fields = value.as_struct()
        cases: list[tuple[str, object]] = [
            ("plain", 1.0),
            ("plain", True),
            ("plain", UNSET),
            ("double_default", 1),
            ("point_default", [1]),
            ("point_default", (1, 2)),
            ("point_default", [UNSET, 0]),
        ]
        for name, raw in cases:
            with self.subTest(name=name, raw=raw):
                with self.assertRaises(EncodeError):
                    fields[name] = raw
        self.assertEqual(value, MutableDynamicValue.default(PRESENCE))

    def test_deleting_and_clearing_optional_fields(self) -> None:
        fields = _presence().as_struct()
        del fields["maybe"]
        self.assertNotIn("maybe", fields)
        with self.assertRaises(KeyError):
            del fields["maybe"]
        fields.clear_optional_field("maybe_point")
        fields.clear_optional_field(10)  # maybe_point again
        self.assertNotIn("maybe_point", fields)
        with self.assertRaises(TypeError):
            del fields["plain"]
        with self.assertRaises(TypeError):
            fields.clear_optional_field("plain")
        with self.assertRaises(KeyError):
            del fields["nope"]
        self.assertIn("plain", fields)

    def test_reset_field_restores_a_fresh_schema_default(self) -> None:
        fields = _presence().as_struct()
        fields.reset_field("ints_default")
        fields.reset_field(2)  # maybe: optional, so absent
        ints_type = _field_type(PRESENCE, "ints_default")
        self.assertEqual(fields["ints_default"], _value(ints_type, [1, 2, 3]))
        self.assertNotIn("maybe", fields)
        fields["ints_default"].as_list().append(4)
        default = MutableDynamicValue.default(PRESENCE).as_struct()
        self.assertEqual(default["ints_default"], _value(ints_type, [1, 2, 3]))

    def test_assignment_copies_the_source(self) -> None:
        source = _value(POINT, [1, 2])
        fields = MutableDynamicValue.default(PRESENCE).as_struct()
        fields["point_default"] = source
        source.as_struct()["x"] = 9
        self.assertEqual(fields["point_default"].as_struct()["x"].as_int(), 1)
        fields["point_default"].as_struct()["y"] = 8
        self.assertEqual(source.as_struct()["y"].as_int(), 2)

    def test_assigning_a_value_into_itself_copies_first(self) -> None:
        value = MutableDynamicValue.default(NESTED)
        value.as_struct()["next"] = value
        inner = value.as_struct()["next"]
        self.assertNotIn("next", inner.as_struct())
        self.assertEqual(inner, MutableDynamicValue.default(NESTED))


class MutableListViewTest(unittest.TestCase):
    def test_item_and_slice_assignment(self) -> None:
        items = _ints(1, 2, 3)
        items[0] = 10
        items[-1] = _value(I32, 30)
        self.assertEqual(items, _ints(10, 2, 30))
        items[1:2] = [20, 21]
        self.assertEqual(items, _ints(10, 20, 21, 30))
        with self.assertRaises(IndexError):
            items[9] = 1

    def test_assignment_is_validated_before_mutation(self) -> None:
        items = _ints(1, 2)
        with self.assertRaises(EncodeError):
            items[0] = "x"
        with self.assertRaises(EncodeError):
            items[0:2] = [3, "x"]
        with self.assertRaises(TypeError):
            items[0] = _value(I64, 1)
        self.assertEqual(items, _ints(1, 2))

    def test_append_extend_insert(self) -> None:
        items = _ints(1)
        items.append(2)
        items.extend([3, _value(I32, 4)])
        items.insert(0, 0)
        items.insert(99, 5)
        self.assertEqual(items, _ints(0, 1, 2, 3, 4, 5))

    def test_extend_is_atomic(self) -> None:
        items = _ints(1)
        with self.assertRaises(EncodeError):
            items.extend([2, "x"])
        with self.assertRaises(TypeError):
            items.extend([2, _value(I64, 3)])
        with self.assertRaises(EncodeError):
            items.append(True)
        self.assertEqual(items, _ints(1))

    def test_extend_with_itself_doubles_the_list(self) -> None:
        points = _value(ListTypeRef(POINT), [[1, 2]]).as_list()
        points.extend(points)
        self.assertEqual(len(points), 2)
        points[0].as_struct()["x"] = 9
        self.assertEqual(points[1].as_struct()["x"].as_int(), 1)

    def test_deletion_and_clear(self) -> None:
        items = _ints(1, 2, 3, 4)
        del items[0]
        del items[1:]
        self.assertEqual(items, _ints(2))
        with self.assertRaises(IndexError):
            del items[5]
        items.clear()
        self.assertEqual(len(items), 0)

    def test_elements_are_mutable_handles(self) -> None:
        points = _value(ListTypeRef(POINT), [[1, 2]]).as_list()
        self.assertIsInstance(points[0], MutableDynamicValue)
        self.assertIsInstance(points[0:1][0], MutableDynamicValue)
        self.assertTrue(all(isinstance(p, MutableDynamicValue) for p in points))
        points[0].as_struct()["x"] = 5
        self.assertEqual(points[0], _value(POINT, [5, 2]))


class MutableMapViewTest(unittest.TestCase):
    def test_set_and_item_assignment(self) -> None:
        map_type = MapTypeRef(STRING, I64)
        counts = _value(map_type, {"a": 1}).as_map()
        counts.set("b", 2)
        counts.set(_value(STRING, "c"), _value(I64, 3))
        counts[_value(STRING, "a")] = 10
        self.assertEqual(counts, _value(map_type, {"a": 10, "b": 2, "c": 3}).as_map())
        self.assertEqual(list(counts), [_value(STRING, k) for k in "abc"])
        with self.assertRaises(TypeError):
            counts[cast(DynamicValue, "d")] = 4

    def test_removal_and_clear(self) -> None:
        counts = _value(MapTypeRef(STRING, I64), {"a": 1, "b": 2, "c": 3}).as_map()
        del counts[_value(STRING, "a")]
        counts.remove("b")
        self.assertEqual(list(counts), [_value(STRING, "c")])
        with self.assertRaises(KeyError):
            counts.remove("zz")
        with self.assertRaises(KeyError):
            del counts[_value(STRING, "zz")]
        with self.assertRaises(TypeError):
            del counts[cast(DynamicValue, "c")]
        with self.assertRaises(TypeError):
            counts.remove(1)
        counts.clear()
        self.assertEqual(len(counts), 0)

    def test_writes_are_validated_before_mutation(self) -> None:
        counts = _value(MapTypeRef(STRING, I64), {"a": 1}).as_map()
        cases: list[tuple[object, object]] = [
            (1, 1),
            ("b", "x"),
            ("b", 2**63),
            (_value(_alias(STRING), "b"), 1),
            ("b", _value(I32, 1)),
        ]
        for key, item in cases:
            with self.subTest(key=key, item=item):
                with self.assertRaises((EncodeError, TypeError)):
                    counts.set(key, item)
        self.assertEqual(list(counts), [_value(STRING, "a")])

    def test_typed_keys_are_validated_against_the_key_type(self) -> None:
        # The aliases share a URI, so their TypeRefs compare equal.
        labels = _value(MapTypeRef(_alias(STRING), I32), {}).as_map()
        frozen = next(iter(_value(SetTypeRef(_alias(I32)), {5: None}).as_set()))
        for key in (_value(_alias(I32), 5), frozen):
            with self.subTest(key=key):
                with self.assertRaises(EncodeError):
                    labels.set(key, 1)
                with self.assertRaises(EncodeError):
                    labels[key] = 1
        self.assertEqual(len(labels), 0)

    def test_values_are_mutable_and_keys_are_read_only(self) -> None:
        lists = _value(MapTypeRef(I32, ListTypeRef(STRING)), {7: ["x"]}).as_map()
        found = lists.lookup(7)
        self.assertIsInstance(found, MutableDynamicValue)
        found.as_list().append("y")
        self.assertEqual(lists.lookup(7), _value(ListTypeRef(STRING), ["x", "y"]))
        for key, item in lists.items():
            self.assertNotIsInstance(key, MutableDynamicValue)
            self.assertIsInstance(item, MutableDynamicValue)
        labels = _value(MapTypeRef(POINT, STRING), {(1, 2): "a"}).as_map()
        point_key = next(iter(labels))
        self.assertNotIsInstance(point_key.as_struct(), MutableStructView)

    def test_keys_are_frozen_at_insertion(self) -> None:
        labels = _value(MapTypeRef(POINT, STRING), {}).as_map()
        source = _value(POINT, [1, 2])
        labels.set(source, "a")
        source.as_struct()["x"] = 9
        self.assertEqual(labels.lookup([1, 2]).as_str(), "a")
        self.assertIsNone(labels.find_key([9, 2]))
        raw = [3, 4]
        labels.set(raw, "b")
        raw[0] = 0
        self.assertEqual(labels.lookup([3, 4]).as_str(), "b")

    def test_entries_from_another_map_can_be_inserted(self) -> None:
        map_type = MapTypeRef(POINT, STRING)
        labels = _value(map_type, {(1, 2): "a"}).as_map()
        other = _value(map_type, {}).as_map()
        for key, item in labels.items():
            other[key] = item
        self.assertEqual(other, labels)


class MutableSetViewTest(unittest.TestCase):
    def test_add_discard_remove_clear(self) -> None:
        names = _value(SetTypeRef(STRING), {"a": None}).as_set()
        names.add("b")
        names.add(_value(STRING, "c"))
        names.add("a")
        self.assertEqual(list(names), [_value(STRING, n) for n in "abc"])
        names.discard("a")
        names.discard("zz")
        names.remove(_value(STRING, "b"))
        with self.assertRaises(KeyError):
            names.remove("zz")
        self.assertEqual(list(names), [_value(STRING, "c")])
        names.clear()
        self.assertEqual(len(names), 0)

    def test_writes_are_validated(self) -> None:
        names = _value(SetTypeRef(STRING), {}).as_set()
        with self.assertRaises(EncodeError):
            names.add(1)
        with self.assertRaises(TypeError):
            names.add(_value(BINARY, b"a"))
        with self.assertRaises(TypeError):
            names.discard(1)
        with self.assertRaises(TypeError):
            names.remove(1)
        self.assertEqual(len(names), 0)

    def test_typed_elements_are_validated_against_the_element_type(self) -> None:
        # The aliases share a URI, so their TypeRefs compare equal.
        names = _value(SetTypeRef(_alias(STRING)), {}).as_set()
        frozen = next(iter(_value(SetTypeRef(_alias(I32)), {5: None}).as_set()))
        for element in (_value(_alias(I32), 5), frozen):
            with self.subTest(element=element):
                with self.assertRaises(EncodeError):
                    names.add(element)
        self.assertEqual(len(names), 0)

    def test_elements_are_read_only_and_frozen_at_insertion(self) -> None:
        points = _value(SetTypeRef(POINT), {}).as_set()
        source = _value(POINT, [1, 2])
        points.add(source)
        source.as_struct()["x"] = 9
        element = next(iter(points))
        self.assertNotIsInstance(element, MutableDynamicValue)
        self.assertEqual(element, _value(POINT, [1, 2]))
        self.assertIsInstance(element.copy(), MutableDynamicValue)

    def test_elements_from_another_set_can_be_added(self) -> None:
        first = _value(SetTypeRef(POINT), {(1, 2): None}).as_set()
        second = _value(SetTypeRef(POINT), {}).as_set()
        for element in first:
            second.add(element)
        self.assertEqual(second, first)


class MutableUnionViewTest(unittest.TestCase):
    def test_set_field_switches_the_active_field(self) -> None:
        choice = _value(CHOICE, []).as_union()
        choice.set_field("number", 5)
        self.assertEqual(choice.field("number").as_int(), 5)
        choice.set_field(3, [1, 2])  # point
        self.assertEqual(choice.field("point"), _value(POINT, [1, 2]))
        self.assertIsNone(choice.get("number"))
        choice.clear()
        self.assertIsNone(choice.active)

    def test_set_field_is_validated_before_switching(self) -> None:
        choice = _value(CHOICE, [1, 5]).as_union()
        cases: list[tuple[str | int, object, type[Exception]]] = [
            ("nope", 1, KeyError),
            (99, 1, KeyError),
            ("text", 1, EncodeError),
            ("number", _value(I64, 1), TypeError),
            ("point", [UNSET, 0], EncodeError),
            (True, 1, TypeError),
        ]
        for selector, item, error in cases:
            with self.subTest(selector=selector, item=item):
                with self.assertRaises(error):
                    choice.set_field(selector, item)
        self.assertEqual(choice, _value(CHOICE, [1, 5]).as_union())

    def test_payload_handles_share_storage(self) -> None:
        choice = _value(CHOICE, [3, [1, 2]]).as_union()
        point = choice.field("point")
        self.assertIsInstance(point, MutableDynamicValue)
        point.as_struct()["x"] = 9
        self.assertEqual(choice.field("point"), _value(POINT, [9, 2]))

    def test_switching_is_visible_through_the_parent(self) -> None:
        value = MutableDynamicValue.default(PRESENCE)
        value.as_struct()["choice_default"].as_union().set_field("number", 1)
        choice = value.as_struct()["choice_default"].as_union()
        self.assertEqual(choice.field("number").as_int(), 1)


class ReferenceSemanticsTest(unittest.TestCase):
    def test_scalar_reads_are_values(self) -> None:
        fields = MutableDynamicValue.default(PRESENCE).as_struct()
        plain = fields["plain"]
        fields["plain"] = 7
        self.assertEqual(plain.as_int(), 0)
        self.assertEqual(fields["plain"].as_int(), 7)

    def test_composite_reads_share_the_stored_object(self) -> None:
        value = MutableDynamicValue.default(PRESENCE)
        point = value.as_struct()["point_default"]
        point.as_struct()["x"] = 10
        self.assertEqual(
            value.as_struct()["point_default"].as_struct()["x"].as_int(), 10
        )
        value.as_struct()["point_default"].as_struct()["y"] = 20
        self.assertEqual(point.as_struct()["y"].as_int(), 20)

    def test_replacing_a_slot_detaches_earlier_handles(self) -> None:
        fields = MutableDynamicValue.default(PRESENCE).as_struct()
        point = fields["point_default"]
        fields["point_default"] = [20, 21]
        self.assertEqual(point, _value(POINT, [1, 2]))
        point.as_struct()["x"] = 5
        self.assertEqual(fields["point_default"], _value(POINT, [20, 21]))

    def test_clearing_and_resetting_detach_earlier_handles(self) -> None:
        fields = _presence().as_struct()
        maybe_point = fields["maybe_point"]
        ints = fields["ints_default"]
        fields.clear_optional_field("maybe_point")
        fields.reset_field("ints_default")
        self.assertEqual(maybe_point, _value(POINT, [7, 8]))
        ints.as_list().append(1)
        self.assertEqual(ints.as_list()[0].as_int(), 9)
        self.assertEqual(len(fields["ints_default"].as_list()), 3)

    def test_switching_a_union_detaches_the_previous_payload(self) -> None:
        choice = _value(CHOICE, [3, [1, 2]]).as_union()
        point = choice.field("point")
        choice.set_field("number", 1)
        self.assertEqual(point, _value(POINT, [1, 2]))

    def test_element_handles_follow_their_object_across_insertion(self) -> None:
        points = _value(ListTypeRef(POINT), [[1, 2]]).as_list()
        first = points[0]
        points.insert(0, [0, 0])
        first.as_struct()["x"] = 9
        self.assertEqual(points[1], _value(POINT, [9, 2]))
        del points[0]
        self.assertEqual(points[0], first)

    def test_map_value_handles_survive_other_insertions(self) -> None:
        lists = _value(MapTypeRef(I32, ListTypeRef(I32)), {1: [1]}).as_map()
        one = lists.lookup(1)
        for key in range(2, 50):
            lists.set(key, [])
        one.as_list().append(2)
        self.assertEqual(lists.lookup(1), _value(ListTypeRef(I32), [1, 2]))

    def test_map_and_set_iteration_detect_structural_changes(self) -> None:
        counts = _value(MapTypeRef(STRING, I64), {"a": 1}).as_map()
        with self.assertRaises(RuntimeError):
            for _ in counts:
                counts.set("b", 2)
        names = _value(SetTypeRef(STRING), {"a": None}).as_set()
        with self.assertRaises(RuntimeError):
            for _ in names:
                names.add("b")  # noqa: B909

    def test_list_iteration_follows_list(self) -> None:
        items = _ints(1)
        seen = []
        for item in items:
            seen.append(item.as_int())
            if len(items) == 1:
                items.append(2)  # noqa: B909
        self.assertEqual(seen, [1, 2])


class ReadOnlyTest(unittest.TestCase):
    def test_read_only_returns_a_live_read_only_handle(self) -> None:
        value = MutableDynamicValue.default(PRESENCE)
        read_only = value.read_only()
        self.assertIs(type(read_only), DynamicValue)
        self.assertIs(type(read_only.read_only()), DynamicValue)
        value.as_struct()["plain"] = 8
        self.assertEqual(read_only.as_struct()["plain"].as_int(), 8)

    def test_mutable_values_are_read_only_values(self) -> None:
        value = _value(I32, 1)
        self.assertIsInstance(value, DynamicValue)
        self.assertNotIsInstance(value.read_only(), MutableDynamicValue)

    def test_children_and_views_are_read_only_at_every_depth(self) -> None:
        fields = _containers().read_only().as_struct()
        handles = [
            fields["ints"],
            fields["ints"].as_list()[0],
            fields["nested_lists"].as_list()[1],
            fields["lists_by_id"].as_map().lookup(7),
            fields["choices"].as_list()[2].as_union().field("point"),
            *fields["deep"].as_list(),
            *fields.values(),
        ]
        for handle in handles:
            with self.subTest(handle=handle):
                self.assertIs(type(handle), DynamicValue)
        views = [
            fields,
            fields["ints"].as_list(),
            fields["counts"].as_map(),
            fields["names"].as_set(),
            fields["choices"].as_list()[0].as_union(),
        ]
        for view in views:
            with self.subTest(view=view):
                self.assertNotIsInstance(view, MUTABLE_VIEWS)

    def test_read_only_views_have_no_mutators(self) -> None:
        fields = _containers().read_only().as_struct()
        cases: list[tuple[object, tuple[str, ...]]] = [
            (
                fields,
                (
                    "__setitem__",
                    "__delitem__",
                    "set_field",
                    "clear_optional_field",
                    "reset_field",
                ),
            ),
            (
                fields["ints"].as_list(),
                ("__setitem__", "__delitem__", "append", "extend", "insert", "clear"),
            ),
            (
                fields["counts"].as_map(),
                ("__setitem__", "__delitem__", "set", "remove", "clear"),
            ),
            (fields["names"].as_set(), ("add", "discard", "remove", "clear")),
            (fields["choices"].as_list()[0].as_union(), ("set_field", "clear")),
        ]
        for view, names in cases:
            for name in names:
                with self.subTest(view=type(view).__name__, name=name):
                    self.assertFalse(hasattr(view, name))

    def test_keys_and_elements_are_read_only_in_mutable_values(self) -> None:
        fields = _containers().as_struct()
        point = next(iter(fields["points"].as_set()))
        weight = next(iter(fields["weights"].as_map()))
        for handle in (point, weight):
            self.assertIs(type(handle), DynamicValue)
        self.assertIs(type(point.as_struct()), StructView)
        self.assertIs(type(point.as_struct()["x"]), DynamicValue)

    def test_copying_a_frozen_key_gives_owned_storage(self) -> None:
        labels = _value(MapTypeRef(POINT, STRING), {(1, 2): "a"}).as_map()
        key = next(iter(labels))
        owned = key.copy()
        owned.as_struct()["x"] = 9
        self.assertEqual(owned, _value(POINT, [9, 2]))
        self.assertEqual(labels.lookup([1, 2]).as_str(), "a")
        self.assertEqual(key.to_datum(), [1, 2])

    def test_frozen_unions_and_nested_containers_are_readable(self) -> None:
        key_type = MapTypeRef(I32, ListTypeRef(CHOICE))
        nested = _value(SetTypeRef(key_type), {FrozenMap({1: ((3, (1, 2)), ())}): None})
        (element,) = nested.as_set()
        choices = element.as_map().lookup(1).as_list()
        self.assertEqual(choices[0].as_union().field("point"), _value(POINT, [1, 2]))
        self.assertIsNone(choices[1].as_union().active)
        self.assertIs(type(choices[0]), DynamicValue)


class LifetimeTest(unittest.TestCase):
    def test_children_and_views_outlive_their_parent(self) -> None:
        parent = _containers()
        ints = parent.as_struct()["ints"].as_list()
        first = ints[0]
        del parent
        gc.collect()
        self.assertEqual(first, _value(I32, 0))
        ints.append(7)
        self.assertEqual(ints[-1], _value(I32, 7))

    def test_read_only_handles_outlive_the_mutable_value(self) -> None:
        read_only = _value(POINT, [1, 2]).read_only()
        gc.collect()
        self.assertEqual(read_only.as_struct()["x"].as_int(), 1)
