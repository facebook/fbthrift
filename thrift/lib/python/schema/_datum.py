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


"""
The plain value representation the codec reads and writes. Values are
immutable, hashable, and not self-describing; a ``TypeRef`` supplies
their meaning.

    bool                     bool
    byte, i16, i32, i64      int (enums too; unknown enum values are kept)
    float                    float, exactly representable in float32
    double                   float
    string                   str (valid UTF-8)
    binary                   bytes
    list<T>                  tuple
    set<T>                   FrozenSet (iterates in insertion order)
    map<K, V>                FrozenMap (an immutable, insertion-ordered dict)
    struct, exception        tuple of field values in TypeSystem field order,
                             with UNSET for an unset optional field
    union                    (field_id, value), or EMPTY_UNION
    opaque alias             the representation of its target type
    any                      thrift-python AnyStruct
"""

from __future__ import annotations

import enum
from collections.abc import Iterable, Iterator
from typing import Final, Literal, NoReturn, TypeVar

from apache.thrift.type.any_rep.thrift_types import AnyStruct
from thrift.lib.python.schema._errors import EncodeError
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
from thrift.lib.python.schema._value_checks import (
    check_binary,
    check_bool,
    check_double,
    check_float32,
    check_int,
    check_string,
    int_in_range,
)
from thrift.lib.python.schema.type_system import (
    EnumTypeRef,
    FieldDefinition,
    InvalidTypeError,
    ListTypeRef,
    MapTypeRef,
    OpaqueAliasTypeRef,
    PresenceQualifier,
    Primitive,
    PrimitiveTypeRef,
    SetTypeRef,
    StructTypeRef,
    TypeRef,
    TypeRefBase,
    UnionTypeRef,
)

_K = TypeVar("_K")
_V = TypeVar("_V")
_T = TypeVar("_T")


class _Sentinel(enum.Enum):
    UNSET = "UNSET"
    EMPTY_UNION = "EMPTY_UNION"

    def __repr__(self) -> str:
        return self.value


# An unset optional struct field.
UNSET: Final = _Sentinel.UNSET
Unset = Literal[_Sentinel.UNSET]
# A union with no active field. Unlike ``UNSET``, it is a value, which
# an optional field or a union field can hold.
EMPTY_UNION: Final = _Sentinel.EMPTY_UNION


class FrozenMap(dict[_K, _V]):
    """A hashable, insertion-ordered ``dict`` with mutation disabled. Equality
    and hashing ignore order. Native code can read it through the dict C-API
    (``PyDict_Next``, ``PyDict_GetItem``)."""

    __slots__ = ("_hash",)
    _hash: int

    def __hash__(self) -> int:
        try:
            return self._hash
        except AttributeError:
            self._hash = hash(frozenset(self.items()))
            return self._hash

    def _immutable(self) -> TypeError:
        return TypeError(f"'{type(self).__name__}' object is immutable")

    def __setitem__(self, key: _K, value: _V) -> NoReturn:
        raise self._immutable()

    def __delitem__(self, key: _K) -> NoReturn:
        raise self._immutable()

    def __ior__(self, other: object) -> NoReturn:
        raise self._immutable()

    def clear(self) -> NoReturn:
        raise self._immutable()

    def pop(self, *args: object) -> NoReturn:
        raise self._immutable()

    def popitem(self) -> NoReturn:
        raise self._immutable()

    def setdefault(self, *args: object) -> NoReturn:
        raise self._immutable()

    def update(self, *args: object, **kwargs: object) -> NoReturn:
        raise self._immutable()

    def copy(self) -> FrozenMap[_K, _V]:
        return self

    def __reduce__(self) -> tuple[type[FrozenMap[_K, _V]], tuple[dict[_K, _V]]]:
        # The default dict pickling would call the disabled __setitem__.
        return (type(self), (dict(self),))

    def __repr__(self) -> str:
        return f"{type(self).__name__}({dict.__repr__(self)})"


class FrozenSet(frozenset[_T]):
    """A ``frozenset`` that iterates in insertion order, so that encoding is
    deterministic (``frozenset`` iterates in hash order, and string hashes
    change per process). A duplicate keeps its first position."""

    __slots__ = ("_order",)
    _order: tuple[_T, ...]

    def __new__(cls, elements: Iterable[_T] = ()) -> FrozenSet[_T]:
        order = tuple(dict.fromkeys(elements))
        instance = super().__new__(cls, order)
        instance._order = order
        return instance

    def __iter__(self) -> Iterator[_T]:
        return iter(self._order)

    def __reduce__(self) -> tuple[type[FrozenSet[_T]], tuple[tuple[_T, ...]]]:
        return (type(self), (self._order,))

    def __repr__(self) -> str:
        return f"{type(self).__name__}({list(self._order)!r})"


_EMPTY_SET: FrozenSet[object] = FrozenSet()

_PRIMITIVE_DEFAULTS: dict[Primitive, object] = {
    Primitive.BOOL: False,
    Primitive.BYTE: 0,
    Primitive.I16: 0,
    Primitive.I32: 0,
    Primitive.I64: 0,
    Primitive.FLOAT: 0.0,
    Primitive.DOUBLE: 0.0,
    Primitive.STRING: "",
    Primitive.BINARY: b"",
}

INT_BITS: dict[Primitive, int] = {
    Primitive.BYTE: 8,
    Primitive.I16: 16,
    Primitive.I32: 32,
    Primitive.I64: 64,
}


def resolved(type_ref: TypeRefBase) -> TypeRef:
    """``type_ref`` as a ``TypeRef``. Container element types are typed
    ``TypeRefBase`` because builder input may hold unresolved references; a
    built type system holds only ``TypeRef``s."""
    if not isinstance(type_ref, TypeRef):
        raise InvalidTypeError(f"unresolved type {type_ref!r}")
    return type_ref


def true_type(type_ref: TypeRef) -> TypeRef:
    """``type_ref`` with an opaque alias replaced by its target. Alias
    targets are never user-defined, so one step suffices."""
    if isinstance(type_ref, OpaqueAliasTypeRef):
        return type_ref.node.target_type
    return type_ref


def type_name(type_ref: TypeRef) -> str:
    """A short name for error messages."""
    match type_ref:
        case PrimitiveTypeRef():
            return type_ref.primitive.name.lower()
        case ListTypeRef():
            return f"list<{type_name(resolved(type_ref.element_type))}>"
        case SetTypeRef():
            return f"set<{type_name(resolved(type_ref.element_type))}>"
        case MapTypeRef():
            key = type_name(resolved(type_ref.key_type))
            return f"map<{key}, {type_name(resolved(type_ref.value_type))}>"
        case _:
            return type_ref.node.uri


# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------


def default_value(type_ref: TypeRef) -> object:
    """The default value of ``type_ref``: zero or empty. A struct holds each
    field's default (``field_default``)."""
    match true_type(type_ref):
        case PrimitiveTypeRef(primitive=Primitive.ANY):
            return AnyStruct()
        case PrimitiveTypeRef(primitive=primitive):
            return _PRIMITIVE_DEFAULTS[primitive]
        case EnumTypeRef():
            return 0
        case ListTypeRef():
            return ()
        case SetTypeRef():
            return _EMPTY_SET
        case MapTypeRef():
            return FrozenMap()
        case StructTypeRef(node=node):
            return tuple(field_default(f) for f in node.fields)
        case UnionTypeRef():
            return EMPTY_UNION
        case target:
            raise InvalidTypeError(f"opaque alias of user-defined type {target!r}")


def field_default(field: FieldDefinition) -> object:
    """The value a struct field holds when it is absent: ``UNSET`` if it is
    optional, else its custom default, else its type's default."""
    if field.presence is PresenceQualifier.OPTIONAL:
        return UNSET
    if field.custom_default is not None:
        return value_from_record(field.custom_default, field.type)
    return default_value(field.type)


def _record_mismatch(record: SerializableRecord, type_ref: TypeRef) -> InvalidTypeError:
    return InvalidTypeError(
        f"custom default {record!r} does not fit type '{type_name(type_ref)}'"
    )


def _int_from_record(
    record: SerializableRecord,
    record_type: type[Int8Record | Int16Record | Int32Record | Int64Record],
    bits: int,
    type_ref: TypeRef,
) -> int:
    if not isinstance(record, record_type) or not int_in_range(record.value, bits):
        raise _record_mismatch(record, type_ref)
    return record.value


_INT_RECORDS: dict[
    Primitive, type[Int8Record | Int16Record | Int32Record | Int64Record]
] = {
    Primitive.BYTE: Int8Record,
    Primitive.I16: Int16Record,
    Primitive.I32: Int32Record,
    Primitive.I64: Int64Record,
}

_ScalarRecord = (
    BoolRecord | Float32Record | Float64Record | TextRecord | ByteArrayRecord
)

_SCALAR_RECORDS: dict[Primitive, type[_ScalarRecord]] = {
    Primitive.BOOL: BoolRecord,
    Primitive.FLOAT: Float32Record,
    Primitive.DOUBLE: Float64Record,
    Primitive.STRING: TextRecord,
    Primitive.BINARY: ByteArrayRecord,
}


def _primitive_from_record(
    record: SerializableRecord, primitive: Primitive, type_ref: TypeRef
) -> object:
    int_record = _INT_RECORDS.get(primitive)
    if int_record is not None:
        return _int_from_record(record, int_record, INT_BITS[primitive], type_ref)
    scalar_record = _SCALAR_RECORDS.get(primitive)
    if scalar_record is None or not isinstance(record, scalar_record):
        raise _record_mismatch(record, type_ref)
    return record.value


def value_from_record(record: SerializableRecord, type_ref: TypeRef) -> object:
    """The value a ``SerializableRecord`` denotes as ``type_ref``, for custom
    defaults. A struct record may omit fields, which take their defaults.
    Raises ``InvalidTypeError`` if the record does not fit."""
    target = true_type(type_ref)
    match target:
        case PrimitiveTypeRef(primitive=primitive):
            return _primitive_from_record(record, primitive, type_ref)
        case EnumTypeRef():
            return _int_from_record(record, Int32Record, 32, type_ref)
        case ListTypeRef() if isinstance(record, ListRecord):
            element_type = resolved(target.element_type)
            return tuple(value_from_record(e, element_type) for e in record.elements)
        case SetTypeRef() if isinstance(record, SetRecord):
            element_type = resolved(target.element_type)
            return FrozenSet(
                value_from_record(e, element_type) for e in record.elements
            )
        case MapTypeRef() if isinstance(record, MapRecord):
            key_type = resolved(target.key_type)
            value_type = resolved(target.value_type)
            return FrozenMap(
                {
                    value_from_record(k, key_type): value_from_record(v, value_type)
                    for k, v in record.entries
                }
            )
        case StructTypeRef() if isinstance(record, FieldSetRecord):
            return tuple(
                value_from_record(record.fields[f.identity.id], f.type)
                if f.identity.id in record.fields
                else field_default(f)
                for f in target.node.fields
            )
        case UnionTypeRef() if isinstance(record, FieldSetRecord):
            return _union_from_record(record, target)
        case _:
            raise _record_mismatch(record, type_ref)


def _union_from_record(record: FieldSetRecord, type_ref: UnionTypeRef) -> object:
    if not record.fields:
        return EMPTY_UNION
    if len(record.fields) > 1:
        raise InvalidTypeError(
            f"union record {record!r} has more than one active field"
        )
    ((field_id, field_record),) = record.fields.items()
    field = type_ref.node.field_by_id(field_id)
    if field is None:
        raise InvalidTypeError(
            f"unknown field id {field_id} in record for union '{type_ref.node.uri}'"
        )
    return (field_id, value_from_record(field_record, field.type))


# ---------------------------------------------------------------------------
# Validation
# ---------------------------------------------------------------------------


def check_value(value: object, type_ref: TypeRef) -> None:
    """Raise ``EncodeError`` unless ``value`` is a valid value of
    ``type_ref``. ``UNSET`` is valid only as a struct field: an unset optional
    field, or a non-optional field to be written as its default."""
    target = true_type(type_ref)
    name = type_name(type_ref)
    match target:
        case PrimitiveTypeRef(primitive=primitive):
            _check_primitive(value, primitive, name)
        case EnumTypeRef():
            check_int(value, 32, name)
        case ListTypeRef():
            element_type = resolved(target.element_type)
            for element in expect_instance(value, tuple, name):
                check_value(element, element_type)
        case SetTypeRef():
            element_type = resolved(target.element_type)
            for element in expect_instance(value, FrozenSet, name):
                check_value(element, element_type)
        case MapTypeRef():
            key_type = resolved(target.key_type)
            value_type = resolved(target.value_type)
            for key, item in expect_instance(value, FrozenMap, name).items():
                check_value(key, key_type)
                check_value(item, value_type)
        case StructTypeRef():
            _check_struct(value, target)
        case UnionTypeRef():
            _check_union(value, target)
        case _:
            raise InvalidTypeError(f"opaque alias of user-defined type {target!r}")


def _check_primitive(value: object, primitive: Primitive, name: str) -> None:
    match primitive:
        case Primitive.BOOL:
            check_bool(value, name)
        case Primitive.BYTE | Primitive.I16 | Primitive.I32 | Primitive.I64:
            check_int(value, INT_BITS[primitive], name)
        case Primitive.FLOAT:
            check_float32(value, name)
        case Primitive.DOUBLE:
            check_double(value, name)
        case Primitive.STRING:
            check_string(value, name)
        case Primitive.BINARY:
            check_binary(value, name)
        case Primitive.ANY:
            check_any(value)


def check_any(value: object) -> AnyStruct:
    if not isinstance(value, AnyStruct):
        raise EncodeError(
            f"value {value!r} of Python type '{type(value).__name__}' does not fit "
            "type 'any' (an AnyStruct)"
        )
    return value


_C = TypeVar("_C")


def expect_instance(value: object, kind: type[_C], name: str) -> _C:
    if not isinstance(value, kind):
        raise EncodeError(
            f"value {value!r} of Python type '{type(value).__name__}' does not fit "
            f"type '{name}' (a {kind.__name__})"
        )
    return value


def check_struct_shape(value: object, type_ref: StructTypeRef) -> tuple[object, ...]:
    fields = type_ref.node.fields
    if not isinstance(value, tuple) or len(value) != len(fields):
        raise EncodeError(
            f"value {value!r} does not fit struct '{type_ref.node.uri}': expected a "
            f"tuple of {len(fields)} field values"
        )
    return value


def check_union_shape(
    value: object, type_ref: UnionTypeRef
) -> tuple[FieldDefinition, object] | None:
    """The active field and its value, or ``None`` for an empty union."""
    if value is EMPTY_UNION:
        return None
    if isinstance(value, tuple) and len(value) == 2:
        field_id, field_value = value
        if isinstance(field_id, int) and not isinstance(field_id, bool):
            field = type_ref.node.field_by_id(field_id)
            if field is not None and field_value is not UNSET:
                return field, field_value
    raise EncodeError(
        f"value {value!r} does not fit union '{type_ref.node.uri}': expected "
        "EMPTY_UNION or a (field_id, value) pair with a known field id"
    )


def field_error(
    type_ref: StructTypeRef | UnionTypeRef, field: FieldDefinition, error: EncodeError
) -> EncodeError:
    """``error`` prefixed with the field it occurred in."""
    path = f"{type_ref.node.uri.rpartition('/')[2]}.{field.identity.name}"
    return EncodeError(f"{path}: {error}")


def _check_struct(value: object, type_ref: StructTypeRef) -> None:
    values = check_struct_shape(value, type_ref)
    for field, field_value in zip(type_ref.node.fields, values):
        if field_value is UNSET:
            continue
        try:
            check_value(field_value, field.type)
        except EncodeError as e:
            raise field_error(type_ref, field, e) from None


def _check_union(value: object, type_ref: UnionTypeRef) -> None:
    active = check_union_shape(value, type_ref)
    if active is None:
        return
    field, field_value = active
    try:
        check_value(field_value, field.type)
    except EncodeError as e:
        raise field_error(type_ref, field, e) from None
