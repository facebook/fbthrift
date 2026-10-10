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
    check_value,
    EMPTY_UNION,
    FrozenMap,
    FrozenSet,
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


def _alias(target: TypeRef) -> OpaqueAliasTypeRef:
    return OpaqueAliasTypeRef(
        OpaqueAliasNode(uri="test.dev/codec/A", target_type=target)
    )


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
            ((1, 2), _alias(ListTypeRef(I32))),
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
            ([1, 2], ListTypeRef(I32)),
            ((1, "2"), ListTypeRef(I32)),
            (frozenset({1}), SetTypeRef(I32)),
            ({1: 2}, MapTypeRef(I32, I32)),
            (FrozenMap({1: "x"}), MapTypeRef(I32, I32)),
            (FrozenMap({"x": 1}), MapTypeRef(I32, I32)),
            ((1,), POINT),  # wrong length
            ((1, 2, 3), POINT),
            ([1, 2], POINT),
            ((99, 1), CHOICE),  # unknown field id
            ((1, "not an i32"), CHOICE),
            ((1,), CHOICE),
            ((1, UNSET), CHOICE),
            (None, CHOICE),
            ((1, 2), _alias(STRING)),
        ]
        for value, type_ref in cases:
            with self.subTest(value=value, type_ref=type_ref):
                with self.assertRaises(EncodeError):
                    check_value(value, type_ref)

    def test_unset_is_allowed_only_in_struct_fields(self) -> None:
        check_value((UNSET, UNSET), POINT)  # written as defaults
        for value, type_ref in [
            (UNSET, I32),
            (UNSET, POINT),
            (UNSET, CHOICE),
            ((1, UNSET), WRAPPER),
            ((UNSET,), ListTypeRef(CHOICE)),
            ((UNSET,), ListTypeRef(I32)),
            (FrozenSet([UNSET]), SetTypeRef(I32)),
            (FrozenMap({UNSET: 1}), MapTypeRef(I32, I32)),
            (FrozenMap({1: UNSET}), MapTypeRef(I32, I32)),
        ]:
            with self.subTest(value=value, type_ref=type_ref):
                with self.assertRaises(EncodeError):
                    check_value(value, type_ref)

    def test_empty_union_is_a_union_value(self) -> None:
        check_value(EMPTY_UNION, CHOICE)
        check_value((1, EMPTY_UNION), WRAPPER)
        check_value((EMPTY_UNION,), ListTypeRef(CHOICE))
        for value, type_ref in [
            (EMPTY_UNION, I32),
            (EMPTY_UNION, POINT),
            ((1, EMPTY_UNION), CHOICE),
        ]:
            with self.subTest(value=value, type_ref=type_ref):
                with self.assertRaises(EncodeError):
                    check_value(value, type_ref)

    def test_struct_field_errors_name_the_field(self) -> None:
        with self.assertRaisesRegex(EncodeError, r"Point\.y"):
            check_value((1, "y"), POINT)

    def test_any_values_are_hashable(self) -> None:
        # So that they can be set elements and map keys.
        elements = FrozenSet([any_of_i32(1), any_of_i32(1), any_of_i32(2)])
        self.assertEqual(len(elements), 2)
        check_value(elements, SetTypeRef(ANY))
