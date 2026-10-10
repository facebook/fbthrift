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

import unittest

import thrift.lib.python.schema.tests.codec_test.thrift_types as fixtures
from apache.thrift.type.any_rep.thrift_types import AnyStruct
from thrift.lib.python.schema._datum import (
    check_value,
    default_value,
    EMPTY_UNION,
    FrozenMap,
    FrozenSet,
    UNSET,
    value_from_record,
)
from thrift.lib.python.schema._record import (
    BoolRecord,
    ByteArrayRecord,
    FieldSetRecord,
    Float32Record,
    Float64Record,
    Int16Record,
    Int32Record,
    Int64Record,
    Int8Record,
    ListRecord,
    MapRecord,
    SerializableRecord,
    SetRecord,
    TextRecord,
)
from thrift.lib.python.schema.tests.datum_testing import (
    ANY,
    BINARY,
    BOOL,
    BYTE,
    DOUBLE,
    FLOAT,
    I16,
    I32,
    I64,
    sample_objects,
    STRING,
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
    SetTypeRef,
    StructNode,
    StructTypeRef,
    TypeRef,
    UnionNode,
    UnionTypeRef,
)

POINT = type_ref_of(fixtures.Point)
CHOICE = type_ref_of(fixtures.Choice)
COLOR = type_ref_of(fixtures.Color)
PRESENCE = type_ref_of(fixtures.Presence)
FAILURE = type_ref_of(fixtures.Failure)


def _field(
    field_id: int,
    name: str,
    type_ref: TypeRef,
    presence: PresenceQualifier = PresenceQualifier.UNQUALIFIED,
    custom_default: SerializableRecord | None = None,
) -> FieldDefinition:
    return FieldDefinition(
        identity=FieldIdentity(field_id, name),
        presence=presence,
        type=type_ref,
        custom_default=custom_default,
    )


def _struct(*fields: FieldDefinition) -> StructTypeRef:
    return StructTypeRef(StructNode(uri="test.dev/codec/S", fields=fields))


def _union(*fields: FieldDefinition) -> UnionTypeRef:
    return UnionTypeRef(UnionNode(uri="test.dev/codec/U", fields=fields))


def _alias(target: TypeRef) -> OpaqueAliasTypeRef:
    return OpaqueAliasTypeRef(
        OpaqueAliasNode(uri="test.dev/codec/A", target_type=target)
    )


class DefaultValueTest(unittest.TestCase):
    def test_primitives(self) -> None:
        cases: list[tuple[TypeRef, object]] = [
            (BOOL, False),
            (BYTE, 0),
            (I16, 0),
            (I32, 0),
            (I64, 0),
            (FLOAT, 0.0),
            (DOUBLE, 0.0),
            (STRING, ""),
            (BINARY, b""),
            (COLOR, 0),
        ]
        for type_ref, expected in cases:
            with self.subTest(type_ref=type_ref):
                actual = default_value(type_ref)
                self.assertEqual(actual, expected)
                self.assertIs(type(actual), type(expected))

    def test_any_is_an_empty_any_struct(self) -> None:
        self.assertEqual(default_value(ANY), AnyStruct())

    def test_containers_are_empty_and_immutable(self) -> None:
        self.assertEqual(default_value(ListTypeRef(I32)), ())
        set_value = default_value(SetTypeRef(I32))
        self.assertIsInstance(set_value, FrozenSet)
        self.assertEqual(set_value, FrozenSet())
        map_value = default_value(MapTypeRef(I32, STRING))
        self.assertIsInstance(map_value, FrozenMap)
        self.assertEqual(map_value, FrozenMap())

    def test_union_is_empty(self) -> None:
        self.assertIs(default_value(CHOICE), EMPTY_UNION)

    def test_struct_uses_custom_and_intrinsic_defaults(self) -> None:
        self.assertEqual(
            default_value(PRESENCE),
            (
                0,  # plain
                UNSET,  # maybe: optional
                0,  # terse
                7,  # plain_default
                UNSET,  # maybe_string
                "",  # terse_string
                (1, 2, 3),
                (1, 2),  # point_default
                (2, "hi"),  # choice_default
                UNSET,  # maybe_point
                2,  # color_default: BLUE
                FrozenMap({"a": 1}),
                FrozenSet([4, 5]),
                2.5,
                b"raw",
                0.5,
                True,
            ),
        )

    def test_struct_matches_thrift_python_default(self) -> None:
        for cls in (fixtures.Primitives, fixtures.Containers, fixtures.Presence):
            with self.subTest(cls=cls.__name__):
                type_ref = type_ref_of(cls)
                self.assertEqual(default_value(type_ref), to_value(cls(), type_ref))

    def test_exception(self) -> None:
        self.assertEqual(default_value(FAILURE), ("", 0))

    def test_opaque_alias_uses_its_target(self) -> None:
        self.assertEqual(default_value(_alias(ListTypeRef(I32))), ())
        self.assertEqual(default_value(_alias(I64)), 0)


class ValueFromRecordTest(unittest.TestCase):
    def test_scalars(self) -> None:
        cases: list[tuple[SerializableRecord, TypeRef, object]] = [
            (BoolRecord(True), BOOL, True),
            (Int8Record(-8), BYTE, -8),
            (Int16Record(16), I16, 16),
            (Int32Record(32), I32, 32),
            (Int64Record(-(2**40)), I64, -(2**40)),
            (Float32Record(0.1), FLOAT, 0.10000000149011612),
            (Float64Record(0.1), DOUBLE, 0.1),
            (TextRecord("text"), STRING, "text"),
            (ByteArrayRecord(b"\x00"), BINARY, b"\x00"),
            (Int32Record(99), COLOR, 99),
        ]
        for record, type_ref, expected in cases:
            with self.subTest(record=record):
                self.assertEqual(value_from_record(record, type_ref), expected)

    def test_containers(self) -> None:
        self.assertEqual(
            value_from_record(
                ListRecord([Int32Record(1), Int32Record(2)]), ListTypeRef(I32)
            ),
            (1, 2),
        )
        set_value = value_from_record(
            SetRecord([TextRecord("b"), TextRecord("a")]), SetTypeRef(STRING)
        )
        assert isinstance(set_value, FrozenSet)
        self.assertEqual(list(set_value), ["b", "a"])
        map_value = value_from_record(
            MapRecord([(Int16Record(1), BoolRecord(False))]), MapTypeRef(I16, BOOL)
        )
        self.assertIsInstance(map_value, FrozenMap)
        self.assertEqual(map_value, FrozenMap({1: False}))

    def test_struct_fills_missing_fields_with_defaults(self) -> None:
        record = FieldSetRecord({4: Int32Record(40), 2: Int32Record(20)})
        defaults = default_value(PRESENCE)
        assert isinstance(defaults, tuple)
        expected = list(defaults)
        expected[3] = 40
        expected[1] = 20
        self.assertEqual(value_from_record(record, PRESENCE), tuple(expected))

    def test_union(self) -> None:
        self.assertIs(value_from_record(FieldSetRecord({}), CHOICE), EMPTY_UNION)
        self.assertEqual(
            value_from_record(FieldSetRecord({2: TextRecord("x")}), CHOICE), (2, "x")
        )

    def test_opaque_alias_uses_its_target(self) -> None:
        self.assertEqual(value_from_record(Int64Record(5), _alias(I64)), 5)

    def test_rejects_records_that_do_not_fit(self) -> None:
        cases: list[tuple[SerializableRecord, TypeRef]] = [
            (Int32Record(1), I64),
            (TextRecord("1"), BINARY),
            (Int8Record(300), BYTE),
            (ListRecord([]), SetTypeRef(I32)),
            (FieldSetRecord({1: Int32Record(1), 2: TextRecord("x")}), CHOICE),
            (FieldSetRecord({99: Int32Record(1)}), CHOICE),
            (FieldSetRecord({1: TextRecord("not an i32")}), POINT),
            (FieldSetRecord({}), ANY),
        ]
        for record, type_ref in cases:
            with self.subTest(record=record, type_ref=type_ref):
                with self.assertRaises(InvalidTypeError):
                    value_from_record(record, type_ref)


class DatumInteroperabilityTest(unittest.TestCase):
    def test_accepts_every_sample(self) -> None:
        for name, obj in sample_objects():
            with self.subTest(name=name):
                type_ref = type_ref_of(type(obj))
                check_value(to_value(obj, type_ref), type_ref)

    def test_custom_defaults_of_custom_struct_nodes(self) -> None:
        struct = _struct(
            _field(1, "a", I32, custom_default=Int32Record(5)),
            _field(2, "b", STRING, PresenceQualifier.OPTIONAL),
        )
        self.assertEqual(default_value(struct), (5, UNSET))
        union = _union(_field(1, "a", I32, PresenceQualifier.OPTIONAL))
        self.assertIs(default_value(union), EMPTY_UNION)
