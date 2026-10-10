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

# TypeRefs, generated fixtures, and conversion to the datum representation.

from __future__ import annotations

import functools
from collections.abc import Iterable, Mapping

import thrift.lib.python.schema.tests.codec_test.thrift_types as fixtures
from apache.thrift.type.any_rep.thrift_types import AnyStruct
from apache.thrift.type.standard.thrift_types import StandardProtocol, TypeName, Void
from apache.thrift.type.type_rep.thrift_types import ProtocolUnion, TypeStruct
from folly.iobuf import IOBuf
from thrift.lib.python.schema._datum import EMPTY_UNION, FrozenMap, FrozenSet, UNSET
from thrift.lib.python.schema.schema_registry import SchemaRegistry
from thrift.lib.python.schema.type_system import (
    _type_ref_for_node,
    EnumTypeRef,
    ListTypeRef,
    MapTypeRef,
    OpaqueAliasTypeRef,
    Primitive,
    PrimitiveTypeRef,
    SetTypeRef,
    StructTypeRef,
    TypeRef,
    TypeRefBase,
    UnionTypeRef,
)
from thrift.python.exceptions import GeneratedError
from thrift.python.types import BadEnum, Enum, StructOrUnion, Union

BOOL = PrimitiveTypeRef(Primitive.BOOL)
BYTE = PrimitiveTypeRef(Primitive.BYTE)
I16 = PrimitiveTypeRef(Primitive.I16)
I32 = PrimitiveTypeRef(Primitive.I32)
I64 = PrimitiveTypeRef(Primitive.I64)
FLOAT = PrimitiveTypeRef(Primitive.FLOAT)
DOUBLE = PrimitiveTypeRef(Primitive.DOUBLE)
STRING = PrimitiveTypeRef(Primitive.STRING)
BINARY = PrimitiveTypeRef(Primitive.BINARY)
ANY = PrimitiveTypeRef(Primitive.ANY)


@functools.cache
def registry() -> SchemaRegistry:
    return SchemaRegistry()


def type_ref_of(cls: type[object]) -> TypeRef:
    """The `TypeRef` of a generated class, from the schema registry."""
    get_uri = getattr(cls, "__get_thrift_uri__", None)
    assert get_uri is not None, f"{cls!r} has no thrift URI"
    return _type_ref_for_node(registry().get_user_defined_type_or_throw(get_uri()))


def struct_type_of(cls: type[object]) -> StructTypeRef:
    type_ref = type_ref_of(cls)
    assert isinstance(type_ref, StructTypeRef), type_ref
    return type_ref


def resolved(type_ref: TypeRefBase) -> TypeRef:
    assert isinstance(type_ref, TypeRef), type_ref
    return type_ref


@functools.cache
def _python_field_names(cls: type[object]) -> Mapping[int, str]:
    get_reflection = getattr(cls, "__get_reflection__", None)
    assert get_reflection is not None, f"{cls!r} has no reflection"
    return {field.id: field.py_name for field in get_reflection().fields}


def to_value(obj: object, type_ref: TypeRef) -> object:
    """A thrift-python value in the codec's value representation."""
    match type_ref:
        case PrimitiveTypeRef() | EnumTypeRef():
            return obj.value if isinstance(obj, (Enum, BadEnum)) else obj
        case ListTypeRef():
            assert isinstance(obj, Iterable)
            element_type = resolved(type_ref.element_type)
            return tuple(to_value(e, element_type) for e in obj)
        case SetTypeRef():
            assert isinstance(obj, Iterable)
            element_type = resolved(type_ref.element_type)
            return FrozenSet(to_value(e, element_type) for e in obj)
        case MapTypeRef():
            assert isinstance(obj, Mapping)
            key_type = resolved(type_ref.key_type)
            value_type = resolved(type_ref.value_type)
            return FrozenMap(
                {to_value(k, key_type): to_value(v, value_type) for k, v in obj.items()}
            )
        case StructTypeRef():
            names = _python_field_names(type(obj))
            return tuple(
                UNSET
                if (field_value := getattr(obj, names[f.identity.id])) is None
                else to_value(field_value, f.type)
                for f in type_ref.node.fields
            )
        case UnionTypeRef():
            assert isinstance(obj, Union)
            field_id = obj.fbthrift_current_field.value
            field = type_ref.node.field_by_id(field_id)
            if field is None:
                return EMPTY_UNION
            return (field_id, to_value(obj.fbthrift_current_value, field.type))
        case OpaqueAliasTypeRef():
            return to_value(obj, type_ref.node.target_type)
        case _:
            raise AssertionError(f"unexpected type {type_ref!r}")


Generated = StructOrUnion | GeneratedError


def any_of_i32(value: int) -> AnyStruct:
    """An `AnyStruct` holding a Compact-encoded i32."""
    zigzag = (value << 1) ^ (value >> 31)
    data = bytearray()
    while zigzag > 0x7F:
        data.append((zigzag & 0x7F) | 0x80)
        zigzag >>= 7
    data.append(zigzag)
    return AnyStruct(
        type=TypeStruct(name=TypeName(i32Type=Void.Unused)),
        protocol=ProtocolUnion(standard=StandardProtocol.Compact),
        data=IOBuf(bytes(data)),
    )


# ---------------------------------------------------------------------------
# Sample values: thrift-python objects covering every fixture type.
# ---------------------------------------------------------------------------


def primitives() -> fixtures.Primitives:
    return fixtures.Primitives(
        bool_field=True,
        byte_field=-128,
        i16_field=-32768,
        i32_field=2**31 - 1,
        i64_field=-(2**63),
        float_field=1.5,
        double_field=-0.1,
        string_field='héllo ☃ "quoted"\n',
        binary_field=b"\x00\xff\x80bin",
        enum_field=fixtures.Color.BLUE,
    )


def containers() -> fixtures.Containers:
    return fixtures.Containers(
        ints=[0, -1, 1, 2**31 - 1, -(2**31)] + list(range(20)),
        bools=[True, False] * 9,
        names={"zeta", "alpha", "mu"},
        counts={"a": 1, "b": -(2**63), "c": 2**63 - 1},
        nested_lists=[[], [1], [2, 3]],
        lists_by_id={7: ["x", "y"], -1: []},
        deep=[{"k": {1, -2}}, {}],
        points={fixtures.Point(x=1, y=2), fixtures.Point(x=-3, y=4)},
        choices=[
            fixtures.Choice(number=5),
            fixtures.Choice(text="t"),
            fixtures.Choice(point=fixtures.Point(x=9, y=8)),
            fixtures.Choice(flags=[True, False, True]),
            fixtures.Choice(),
        ],
        weights={fixtures.Color.RED: 0.25, fixtures.Color.GREEN: -2.0},
        floats=[0.5, -1.25, 3.0],
        flags_by_blob={b"": True, b"\x01\x02": False},
        bytes_by_flag={True: 1, False: -1},
        names_by_weight={1.5: "one and a half", -0.25: "negative"},
        blobs=[b"", b"abc", b"\x00"],
        colors=[fixtures.Color.GREEN, fixtures.Color.RED],
        failures=[fixtures.Failure(reason="bad", code=3)],
    )


def presence() -> fixtures.Presence:
    """Every terse field differs from its default, so thrift-python writes it."""
    return fixtures.Presence(
        plain=1,
        maybe=2,
        terse=3,
        plain_default=4,
        maybe_string="set",
        terse_string="terse",
        ints_default=[9],
        point_default=fixtures.Point(x=5, y=6),
        choice_default=fixtures.Choice(number=1),
        maybe_point=fixtures.Point(x=7, y=8),
        color_default=fixtures.Color.RED,
        map_default={"z": 26},
        set_default={6},
        double_default=1.0,
        binary_default=b"cooked",
        float_default=0.25,
        bool_default=False,
    )


def field_ids() -> fixtures.FieldIds:
    return fixtures.FieldIds(
        one=1, seventeen=17, two=2, negative=-5, far_bool=True, max_id=2**40
    )


def with_any() -> fixtures.WithAny:
    return fixtures.WithAny(id=1, payload=any_of_i32(-3))


def nested(depth: int = 3) -> fixtures.Nested:
    result = fixtures.Nested(
        primitives=primitives(),
        failure=fixtures.Failure(reason="leaf", code=-1),
        choice=fixtures.Choice(text="leaf"),
    )
    for level in range(depth - 1):
        result = fixtures.Nested(
            containers=containers() if level == 0 else fixtures.Containers(),
            next=result,
            choice=fixtures.Choice(number=level),
        )
    return result


def flags() -> fixtures.Flags:
    return fixtures.Flags(flag=True, many=[i % 3 == 0 for i in range(20)])


# SimpleJSON cannot write struct or container map keys
NOT_JSON_REPRESENTABLE: frozenset[str] = frozenset({"struct_keyed_map"})


def sample_objects() -> list[tuple[str, Generated]]:
    return [
        ("primitives", primitives()),
        ("default_primitives", fixtures.Primitives()),
        ("point", fixtures.Point(x=-1, y=1)),
        ("choice_number", fixtures.Choice(number=-7)),
        ("choice_point", fixtures.Choice(point=fixtures.Point(x=1, y=2))),
        ("empty_choice", fixtures.Choice()),
        ("failure", fixtures.Failure(reason="why", code=500)),
        ("containers", containers()),
        ("empty_containers", fixtures.Containers()),
        (
            "struct_keyed_map",
            fixtures.Containers(
                labels={
                    fixtures.Point(x=0, y=0): "origin",
                    fixtures.Point(x=1, y=-1): "diagonal",
                }
            ),
        ),
        ("presence", presence()),
        ("field_ids", field_ids()),
        ("with_any", with_any()),
        ("nested", nested()),
        ("flags", flags()),
        (
            "empty_unions",
            fixtures.EmptyUnions(
                maybe_choice=fixtures.Choice(),
                wrapper=fixtures.Wrapper(choice=fixtures.Choice()),
            ),
        ),
        ("default_empty_unions", fixtures.EmptyUnions()),
    ]
