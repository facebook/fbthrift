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
import unittest

from apache.thrift.type.any_rep.thrift_types import AnyStruct
from thrift.lib.python.schema._datum import (
    check_frozen_key,
    check_value,
    copy_value,
    freeze_key,
    FrozenMap,
    UNSET,
)
from thrift.lib.python.schema._errors import EncodeError
from thrift.lib.python.schema.tests.datum_testing import (
    ANY,
    any_of_i32,
    BINARY,
    BOOL,
    BYTE,
    DOUBLE,
    FLOAT,
    I16,
    I32,
    I64,
    STRING,
)
from thrift.lib.python.schema.type_system import (
    EnumNode,
    EnumTypeRef,
    FieldDefinition,
    FieldIdentity,
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


POINT = StructTypeRef(
    StructNode(
        uri="test.dev/codec/Point",
        fields=[
            FieldDefinition(
                identity=FieldIdentity(1, "x"),
                presence=PresenceQualifier.UNQUALIFIED,
                type=I32,
            ),
            FieldDefinition(
                identity=FieldIdentity(2, "y"),
                presence=PresenceQualifier.UNQUALIFIED,
                type=I32,
            ),
        ],
    )
)
CHOICE = UnionTypeRef(
    UnionNode(
        uri="test.dev/codec/Choice",
        fields=[
            FieldDefinition(
                identity=FieldIdentity(1, "number"),
                presence=PresenceQualifier.OPTIONAL,
                type=I32,
            ),
            FieldDefinition(
                identity=FieldIdentity(2, "text"),
                presence=PresenceQualifier.OPTIONAL,
                type=STRING,
            ),
            FieldDefinition(
                identity=FieldIdentity(3, "point"),
                presence=PresenceQualifier.OPTIONAL,
                type=POINT,
            ),
            FieldDefinition(
                identity=FieldIdentity(4, "flags"),
                presence=PresenceQualifier.OPTIONAL,
                type=ListTypeRef(BOOL),
            ),
        ],
    )
)
WRAPPER = UnionTypeRef(
    UnionNode(
        uri="test.dev/codec/Wrapper",
        fields=[
            FieldDefinition(
                identity=FieldIdentity(1, "choice"),
                presence=PresenceQualifier.OPTIONAL,
                type=CHOICE,
            ),
            FieldDefinition(
                identity=FieldIdentity(2, "number"),
                presence=PresenceQualifier.OPTIONAL,
                type=I32,
            ),
        ],
    )
)
COLOR = EnumTypeRef(EnumNode(uri="test.dev/codec/Color"))


MIXED = StructTypeRef(
    StructNode(
        uri="test.dev/codec/Mixed",
        fields=[
            FieldDefinition(
                identity=FieldIdentity(1, "plain"),
                presence=PresenceQualifier.UNQUALIFIED,
                type=I32,
            ),
            FieldDefinition(
                identity=FieldIdentity(2, "maybe"),
                presence=PresenceQualifier.OPTIONAL,
                type=I32,
            ),
            FieldDefinition(
                identity=FieldIdentity(3, "terse"),
                presence=PresenceQualifier.TERSE,
                type=I32,
            ),
            FieldDefinition(
                identity=FieldIdentity(4, "point"),
                presence=PresenceQualifier.UNQUALIFIED,
                type=POINT,
            ),
        ],
    )
)


def _alias(target: TypeRef) -> OpaqueAliasTypeRef:
    return OpaqueAliasTypeRef(
        OpaqueAliasNode(uri="test.dev/codec/A", target_type=target)
    )


def _recursive_type() -> StructTypeRef:
    node = StructNode(uri="test.dev/ownership/Recursive")
    type_ref = StructTypeRef(node)
    node._set_fields(
        [
            FieldDefinition(
                identity=FieldIdentity(1, "next"),
                presence=PresenceQualifier.OPTIONAL,
                type=type_ref,
            )
        ]
    )
    return type_ref


class CheckValueTest(unittest.TestCase):
    def test_accepts_scalars_of_each_kind(self) -> None:
        cases: list[tuple[object, TypeRef]] = [
            (True, BOOL),
            (-128, BYTE),
            (2**15 - 1, I16),
            (-(2**31), I32),
            (2**63 - 1, I64),
            (0.5, FLOAT),
            (math.inf, FLOAT),
            (math.nan, FLOAT),
            (0.1, DOUBLE),
            ("☃", STRING),
            (b"\xff", BINARY),
            (12345, COLOR),  # enums are open
            (any_of_i32(1), ANY),
            ([1, 2], _alias(ListTypeRef(I32))),
        ]
        for value, type_ref in cases:
            with self.subTest(value=value, type_ref=type_ref):
                check_value(value, type_ref)

    def test_rejects_values_that_do_not_fit(self) -> None:
        cases: list[tuple[object, TypeRef]] = [
            (1, BOOL),
            (True, I32),
            (128, BYTE),
            (-(2**15) - 1, I16),
            (2**31, I32),
            (2**63, I64),
            (1.0, I64),
            (1, DOUBLE),
            (0.1, FLOAT),  # not exactly representable in float32
            (1e39, FLOAT),
            ("\ud800", STRING),  # a lone surrogate is not UTF-8
            (b"x", STRING),
            (bytearray(b"x"), BINARY),
            ("x", BINARY),
            (2**31, COLOR),
            (AnyStruct, ANY),
            ((1, 2), ListTypeRef(I32)),
            ([1, "2"], ListTypeRef(I32)),
            (frozenset({1}), SetTypeRef(I32)),
            ({1: 2}, SetTypeRef(I32)),
            ({1: "x"}, MapTypeRef(I32, I32)),
            ({"x": 1}, MapTypeRef(I32, I32)),
            ([1], POINT),  # wrong length
            ([1, 2, 3], POINT),
            ((1, 2), POINT),
            ([99, 1], CHOICE),  # unknown field id
            ([1, "not an i32"], CHOICE),
            ((1,), CHOICE),
            ([1, UNSET], CHOICE),
            (None, CHOICE),
            ((1, 2), _alias(STRING)),
        ]
        for value, type_ref in cases:
            with self.subTest(value=value, type_ref=type_ref):
                with self.assertRaises(EncodeError):
                    check_value(value, type_ref)

    def test_schema_distinguishes_shared_container_shapes(self) -> None:
        for value, type_ref in (
            ([1, 2], POINT),
            ([1, 2], ListTypeRef(I32)),
            ({1: 2}, MapTypeRef(I32, I32)),
            ({1: None}, SetTypeRef(I32)),
        ):
            with self.subTest(type_ref=type_ref):
                self.assertIsNone(check_value(value, type_ref))
                self.assertEqual(copy_value(value, type_ref), value)

    def test_unset_is_allowed_only_in_optional_fields(self) -> None:
        for value, type_ref in (
            ([0, UNSET, 0, [0, 0]], MIXED),
            ([1, 2, 3, [4, 5]], MIXED),
            ([], CHOICE),
            ([1, []], WRAPPER),
        ):
            with self.subTest(value=value):
                self.assertIsNone(check_value(value, type_ref))

    def test_rejects_unset_in_other_positions(self) -> None:
        for value, type_ref in (
            (UNSET, I32),
            (UNSET, POINT),
            (UNSET, CHOICE),
            ([UNSET, UNSET], POINT),
            ([UNSET, UNSET, 0, [0, 0]], MIXED),
            ([0, UNSET, UNSET, [0, 0]], MIXED),
            ([0, UNSET, 0, UNSET], MIXED),
            ([[UNSET, 0]], ListTypeRef(POINT)),
            ([UNSET], ListTypeRef(I32)),
            ({UNSET: None}, SetTypeRef(I32)),
            ({(0, UNSET): None}, SetTypeRef(POINT)),
            ({UNSET: 1}, MapTypeRef(I32, I32)),
            ({1: UNSET}, MapTypeRef(I32, I32)),
            ({(UNSET, 0): 1}, MapTypeRef(POINT, I32)),
            ([3, [UNSET, 0]], CHOICE),
        ):
            with self.subTest(value=value, type_ref=type_ref):
                with self.assertRaises(EncodeError):
                    check_value(value, type_ref)

    def test_empty_union_is_a_present_owned_value(self) -> None:
        value = copy_value([0, UNSET, 0, [0, 0]], MIXED)
        self.assertEqual(value, [0, UNSET, 0, [0, 0]])
        for value, type_ref in (
            ([], CHOICE),
            ([1, []], WRAPPER),
            ([[]], ListTypeRef(CHOICE)),
        ):
            with self.subTest(type_ref=type_ref):
                self.assertIsNone(check_value(value, type_ref))
        self.assertEqual(freeze_key([], CHOICE), ())
        self.assertIsNone(check_frozen_key((), CHOICE))
        for value, type_ref in (
            ((), CHOICE),
            ([None, 1], CHOICE),
            ([True, 1], CHOICE),
            ([1, []], CHOICE),
        ):
            with self.subTest(value=value):
                with self.assertRaises(EncodeError):
                    check_value(value, type_ref)

    def test_field_errors_keep_nested_context(self) -> None:
        with self.assertRaisesRegex(EncodeError, r"Point\.y"):
            check_value([1, "y"], POINT)
        with self.assertRaisesRegex(EncodeError, r"Mixed\.point: Point\.y: .*UNSET"):
            check_value([0, UNSET, 0, [0, UNSET]], MIXED)
        with self.assertRaisesRegex(EncodeError, r"Choice\.point: Point\.x: .*UNSET"):
            check_value([3, [UNSET, 0]], CHOICE)

    def test_any_values_can_be_frozen_in_hashed_positions(self) -> None:
        elements = dict.fromkeys(
            freeze_key(value, ANY)
            for value in [any_of_i32(1), any_of_i32(1), any_of_i32(2)]
        )
        assert isinstance(elements, dict)
        self.assertEqual(len(elements), 2)
        self.assertIsNone(check_value(elements, SetTypeRef(ANY)))

    def test_cycle_is_rejected_by_every_traversal(self) -> None:
        type_ref = _recursive_type()
        cyclic: list[object] = [UNSET]
        cyclic[0] = cyclic
        for operation in (copy_value, freeze_key, check_value):
            with self.subTest(operation=operation.__name__):
                with self.assertRaisesRegex(EncodeError, "cyclic"):
                    operation(cyclic, type_ref)

    def test_invalid_input_does_not_modify_source(self) -> None:
        source = [1, True]
        with self.assertRaises(EncodeError):
            copy_value(source, ListTypeRef(I32))
        self.assertEqual(source, [1, True])

    def test_frozen_validation_rejects_mutable_containers(self) -> None:
        for value, type_ref in (
            ([1], ListTypeRef(I32)),
            ({1: 2}, MapTypeRef(I32, I32)),
            ({1: None}, SetTypeRef(I32)),
            (FrozenMap({1: [2]}), MapTypeRef(I32, ListTypeRef(I32))),
            ((4, [True]), CHOICE),
        ):
            with self.subTest(type_ref=type_ref):
                with self.assertRaises(EncodeError):
                    check_frozen_key(value, type_ref)
