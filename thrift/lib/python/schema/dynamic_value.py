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
Schema-typed read-only handles on owned datum (see ``_datum``).

``DynamicValue`` holds a datum that is valid for its ``TypeRef``. Scalars are
read through checked accessors; containers, structs, and unions through views
whose children take their types from the parent's schema.

A scalar read is a value. A struct, container, or union read shares the stored
object. Map keys and set elements are frozen. Presence is canonical: ``UNSET``
stands only for an unset optional struct field, and every other field holds an
explicit value.
"""

from __future__ import annotations

from collections.abc import (
    Callable,
    Container,
    ItemsView,
    Iterable,
    Iterator,
    KeysView,
    Mapping,
    Sequence,
    Set as AbstractSet,
)
from typing import cast, ClassVar, Generic, NoReturn, overload, Self, TypeVar

from apache.thrift.type.any_rep.thrift_types import AnyStruct
from thrift.lib.python.schema._datum import (
    check_union_shape,
    copy_value,
    freeze_key,
    FrozenMap,
    resolved,
    true_type,
    type_name,
    UNSET,
    values_equal,
)
from thrift.lib.python.schema._errors import EncodeError
from thrift.lib.python.schema.type_system import (
    EnumTypeRef,
    FieldDefinition,
    ListTypeRef,
    MapTypeRef,
    Primitive,
    PrimitiveTypeRef,
    SetTypeRef,
    StructNode,
    StructTypeRef,
    TypeRef,
    UnionTypeRef,
)

__all__ = [
    "DynamicValue",
    "FrozenMap",
    "ListView",
    "MapView",
    "SetView",
    "StructView",
    "UNSET",
    "UnionView",
]

_T = TypeVar("_T")
_D = TypeVar("_D", bound="DynamicValue")
_H = TypeVar("_H", bound="DynamicValue", covariant=True)

_INTEGERS: frozenset[Primitive] = frozenset(
    {Primitive.BYTE, Primitive.I16, Primitive.I32, Primitive.I64}
)
_FLOATS: frozenset[Primitive] = frozenset({Primitive.FLOAT, Primitive.DOUBLE})


class DynamicValue:
    """A read-only handle on a datum and the ``TypeRef`` it was declared as.

    Equality compares the ``TypeRef`` and the value; a handle never equals a
    raw Python value and is unhashable."""

    __slots__ = ("_type_ref", "_datum", "_frozen")
    _type_ref: TypeRef
    _datum: object
    _frozen: bool

    def __init__(self, *args: object, **kwargs: object) -> None:
        raise TypeError("DynamicValue is not constructed directly")

    @property
    def type_ref(self) -> TypeRef:
        return self._type_ref

    def expect(self, expected: TypeRef) -> Self:
        """This value, if its ``TypeRef`` equals ``expected``."""
        if self._type_ref != expected:
            raise TypeError(
                f"expected a value of type '{_describe(expected)}', got "
                f"'{type_name(self._type_ref)}'"
            )
        return self

    def to_datum(self) -> object:
        """An owned copy of the datum, sharing no mutable storage."""
        return copy_value(self._datum, resolved(self._type_ref), frozen=self._frozen)

    def as_bool(self) -> bool:
        return cast(bool, self._primitive({Primitive.BOOL}, "bool"))

    def as_int(self) -> int:
        """A byte, i16, i32, i64, or enum value."""
        if isinstance(true_type(self._type_ref), EnumTypeRef):
            return cast(int, self._datum)
        return cast(int, self._primitive(_INTEGERS, "an integer"))

    def as_float(self) -> float:
        """A float or double value."""
        return cast(float, self._primitive(_FLOATS, "a float"))

    def as_str(self) -> str:
        return cast(str, self._primitive({Primitive.STRING}, "a string"))

    def as_bytes(self) -> bytes:
        return cast(bytes, self._primitive({Primitive.BINARY}, "binary"))

    def as_any(self) -> AnyStruct:
        """The ``AnyStruct``, with its payload left encoded."""
        return cast(AnyStruct, self._primitive({Primitive.ANY}, "any"))

    def as_list(self) -> ListView[DynamicValue]:
        target = self._target(ListTypeRef, "a list")
        elements = cast(Sequence[object], self._datum)
        return ListView(resolved(target.element_type), elements, self._frozen)

    def as_set(self) -> SetView:
        target = self._target(SetTypeRef, "a set")
        elements = cast(Mapping[object, None], self._datum)
        return SetView(resolved(target.element_type), elements)

    def as_map(self) -> MapView[DynamicValue]:
        target = self._target(MapTypeRef, "a map")
        entries = cast(Mapping[object, object], self._datum)
        key_type, value_type = resolved(target.key_type), resolved(target.value_type)
        return MapView(key_type, value_type, entries, self._frozen)

    def as_struct(self) -> StructView[DynamicValue]:
        """The present fields of a struct or exception."""
        target = self._target(StructTypeRef, "a struct")
        return StructView(
            target.node, cast(Sequence[object], self._datum), self._frozen
        )

    def as_union(self) -> UnionView[DynamicValue]:
        target = self._target(UnionTypeRef, "a union")
        return UnionView(target, cast(Sequence[object], self._datum), self._frozen)

    def _target(self, kind: type[_T], description: str) -> _T:
        target = true_type(self._type_ref)
        if not isinstance(target, kind):
            self._mismatch(description)
        return target

    def _primitive(self, primitives: Container[Primitive], kind: str) -> object:
        target = true_type(self._type_ref)
        if isinstance(target, PrimitiveTypeRef) and target.primitive in primitives:
            return self._datum
        self._mismatch(kind)

    def _mismatch(self, kind: str) -> NoReturn:
        raise TypeError(
            f"value of type '{type_name(self._type_ref)}' cannot be read as {kind}"
        )

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, DynamicValue):
            return NotImplemented
        return self._type_ref == other._type_ref and values_equal(
            self._datum, other._datum
        )

    def __setattr__(self, name: str, value: object) -> NoReturn:
        raise AttributeError(f"cannot set attribute '{name}' of {type(self).__name__}")

    def __delattr__(self, name: str) -> NoReturn:
        raise AttributeError(
            f"cannot delete attribute '{name}' of {type(self).__name__}"
        )

    def __reduce__(
        self,
    ) -> tuple[Callable[[TypeRef, object], DynamicValue], tuple[TypeRef, object]]:
        # The default reduction would call the disabled __setattr__.
        return (_read_only_value, (self._type_ref, self.to_datum()))

    def __repr__(self) -> str:
        return f"{type(self).__name__}({type_name(self._type_ref)}, {self._datum!r})"


# ---------------------------------------------------------------------------
# Views. Each holds the parent's stored container and the child types its
# schema declares, and wraps children as they are read.
# ---------------------------------------------------------------------------


class _View(Generic[_H]):
    __slots__ = ()
    _HANDLE: ClassVar[type[DynamicValue]] = DynamicValue
    _frozen: bool

    def _child(self, type_ref: TypeRef, stored: object) -> _H:
        return cast(_H, _handle(self._HANDLE, type_ref, stored, self._frozen))


class ListView(_View[_H], Sequence[_H]):
    """The elements of a list value."""

    __slots__ = ("_element_type", "_elements", "_frozen")
    _element_type: TypeRef
    _elements: Sequence[object]

    def __init__(
        self, element_type: TypeRef, elements: Sequence[object], frozen: bool
    ) -> None:
        self._element_type = element_type
        self._elements = elements
        self._frozen = frozen

    @overload
    def __getitem__(self, index: int) -> _H: ...

    @overload
    def __getitem__(self, index: slice) -> tuple[_H, ...]: ...

    def __getitem__(self, index: int | slice) -> _H | tuple[_H, ...]:
        if isinstance(index, slice):
            element_type = self._element_type
            return tuple(self._child(element_type, e) for e in self._elements[index])
        return self._child(self._element_type, self._elements[index])

    def __iter__(self) -> Iterator[_H]:
        element_type = self._element_type
        return (self._child(element_type, e) for e in self._elements)

    def __len__(self) -> int:
        return len(self._elements)

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, ListView):
            return NotImplemented
        return self._element_type == other._element_type and values_equal(
            self._elements, other._elements
        )


class _TypedSetEquality:
    """Equality for a set of typed elements. An operand holding a raw element
    is unequal, rather than raising on an unhashable handle."""

    __slots__ = ()

    def _is_typed(self, element: object) -> bool:
        return isinstance(element, DynamicValue)

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, AbstractSet):
            return NotImplemented
        if not all(self._is_typed(element) for element in other):
            return False
        this = cast(AbstractSet[object], self)
        return len(this) == len(other) and all(e in other for e in this)


class SetView(_TypedSetEquality, AbstractSet[DynamicValue]):
    """The elements of a set value, in insertion order, as read-only
    ``DynamicValue``s. Membership requires a ``DynamicValue``; ``find`` also
    accepts raw datum. Set algebra is not supported."""

    __slots__ = ("_element_type", "_elements")
    _element_type: TypeRef
    _elements: Mapping[object, None]

    def __init__(self, element_type: TypeRef, elements: Mapping[object, None]) -> None:
        self._element_type = element_type
        self._elements = elements

    def __contains__(self, element: object) -> bool:
        return (
            isinstance(element, DynamicValue)
            and element.type_ref == self._element_type
            and _frozen_datum(element) in self._elements
        )

    def __iter__(self) -> Iterator[DynamicValue]:
        element_type = self._element_type
        return (_key_handle(element_type, e) for e in self._elements)

    def __len__(self) -> int:
        return len(self._elements)

    def find(self, probe: object) -> DynamicValue | None:
        """The stored element equal to ``probe`` (raw datum or a
        ``DynamicValue``), or ``None``."""
        stored = _probe_key(probe, self._element_type)
        return _find(self._element_type, self._elements, stored)

    @classmethod
    def _from_iterable(cls, it: Iterable[object]) -> NoReturn:
        raise TypeError("set algebra on a SetView is not supported")


class MapView(_View[_H], Mapping[DynamicValue, _H]):
    """The entries of a map value, in insertion order. Indexing, ``get``, and
    membership require ``DynamicValue`` keys; ``lookup`` and ``find_key`` also
    accept raw datum. Keys are read-only."""

    __slots__ = ("_key_type", "_value_type", "_entries", "_frozen")
    _key_type: TypeRef
    _value_type: TypeRef
    _entries: Mapping[object, object]

    def __init__(
        self,
        key_type: TypeRef,
        value_type: TypeRef,
        entries: Mapping[object, object],
        frozen: bool,
    ) -> None:
        self._key_type = key_type
        self._value_type = value_type
        self._entries = entries
        self._frozen = frozen

    def __getitem__(self, key: DynamicValue) -> _H:
        if not isinstance(key, DynamicValue):
            raise TypeError(
                f"map key {key!r} is not a DynamicValue; use lookup() for raw datum"
            )
        if key.type_ref != self._key_type:
            raise KeyError(key)
        stored = _frozen_datum(key)
        if stored not in self._entries:
            raise KeyError(key)
        return self._child(self._value_type, self._entries[stored])

    def __contains__(self, key: object) -> bool:
        return (
            isinstance(key, DynamicValue)
            and key.type_ref == self._key_type
            and _frozen_datum(key) in self._entries
        )

    def __iter__(self) -> Iterator[DynamicValue]:
        key_type = self._key_type
        return (_key_handle(key_type, k) for k in self._entries)

    def __len__(self) -> int:
        return len(self._entries)

    def keys(self) -> KeysView[DynamicValue]:
        return _MapKeys(self)

    def items(self) -> ItemsView[DynamicValue, _H]:
        return _MapItems(self)

    def lookup(self, probe: object) -> _H:
        """The value for ``probe`` (raw datum or a ``DynamicValue``)."""
        stored = _probe_key(probe, self._key_type)
        if stored not in self._entries:
            raise KeyError(probe)
        return self._child(self._value_type, self._entries[stored])

    def find_key(self, probe: object) -> DynamicValue | None:
        """The stored key equal to ``probe`` (raw datum or a
        ``DynamicValue``), or ``None``."""
        stored = _probe_key(probe, self._key_type)
        return _find(self._key_type, self._entries, stored)

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, MapView):
            return NotImplemented
        return (self._key_type, self._value_type) == (
            other._key_type,
            other._value_type,
        ) and values_equal(self._entries, other._entries)


class _MapKeys(_TypedSetEquality, KeysView[DynamicValue]):
    __slots__ = ()


class _MapItems(_TypedSetEquality, ItemsView[DynamicValue, _H]):
    __slots__ = ()  # MappingView supplies the inherited _mapping slot.
    _mapping: MapView[_H]

    def _is_typed(self, element: object) -> bool:
        return (
            isinstance(element, tuple)
            and len(element) == 2
            and all(isinstance(part, DynamicValue) for part in element)
        )

    def __contains__(self, item: object) -> bool:
        if not self._is_typed(item):
            return False
        key, value = cast(tuple[DynamicValue, DynamicValue], item)
        mapping = self._mapping
        return key in mapping and mapping[key] == value


class StructView(_View[_H], Mapping[str, _H]):
    """The present fields of a struct or exception value, keyed by Thrift
    field name, in schema order. An unset optional field is absent."""

    __slots__ = ("_node", "_values", "_frozen")
    _node: StructNode
    _values: Sequence[object]

    def __init__(
        self, node: StructNode, values: Sequence[object], frozen: bool
    ) -> None:
        self._node = node
        self._values = values
        self._frozen = frozen

    def __getitem__(self, name: str) -> _H:
        return self._present(self._by_name(name), name)

    def by_id(self, field_id: int) -> _H:
        return self._present(self._by_id(field_id), field_id)

    def _by_name(self, name: str) -> FieldDefinition:
        field = self._node.field_by_name(name)
        if field is None:
            raise KeyError(name)
        return field

    def _by_id(self, field_id: int) -> FieldDefinition:
        field = self._node.field_by_id(_field_id(field_id))
        if field is None:
            raise KeyError(field_id)
        return field

    def _index(self, field: FieldDefinition) -> int:
        return self._node.fields.index(field)

    def _present(self, field: FieldDefinition, selector: object) -> _H:
        value = self._values[self._index(field)]
        if value is UNSET:
            raise KeyError(selector)
        return self._child(field.type, value)

    def __iter__(self) -> Iterator[str]:
        return (
            field.identity.name
            for field, value in zip(self._node.fields, self._values)
            if value is not UNSET
        )

    def __len__(self) -> int:
        return sum(value is not UNSET for value in self._values)


class UnionView(_View[_H]):
    """The active field of a union value, if any."""

    __slots__ = ("_union_type", "_datum", "_frozen")
    _union_type: UnionTypeRef
    _datum: Sequence[object]

    def __init__(
        self, union_type: UnionTypeRef, stored: Sequence[object], frozen: bool
    ) -> None:
        self._union_type = union_type
        self._datum = stored
        self._frozen = frozen

    @property
    def active(self) -> tuple[FieldDefinition, _H] | None:
        """The active field and its value, or ``None`` for an empty union."""
        active = check_union_shape(self._datum, self._union_type, frozen=self._frozen)
        if active is None:
            return None
        field, value = active
        return field, self._child(field.type, value)

    def field(self, selector: str | int) -> _H:
        """The value of the field named, or with the id, ``selector``, if it
        is active."""
        field = self._field(selector)
        active = self.active
        if active is None or active[0] is not field:
            raise KeyError(selector)
        return active[1]

    def get(self, selector: str | int) -> _H | None:
        try:
            return self.field(selector)
        except KeyError:
            return None

    def _field(self, selector: str | int) -> FieldDefinition:
        node = self._union_type.node
        if isinstance(selector, str):
            field = node.field_by_name(selector)
        else:
            field = node.field_by_id(_field_id(selector))
        if field is None:
            raise KeyError(selector)
        return field

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, UnionView):
            return NotImplemented
        return self._union_type == other._union_type and values_equal(
            self._datum, other._datum
        )


def _handle(cls: type[_D], type_ref: TypeRef, stored: object, frozen: bool) -> _D:
    """A handle on already validated stored datum."""
    value = object.__new__(cls)
    object.__setattr__(value, "_type_ref", type_ref)
    object.__setattr__(value, "_datum", stored)
    object.__setattr__(value, "_frozen", frozen)
    return value


def _key_handle(type_ref: TypeRef, stored: object) -> DynamicValue:
    return _handle(DynamicValue, type_ref, stored, True)


def _read_only_value(type_ref: TypeRef, datum: object) -> DynamicValue:
    stored = copy_value(datum, resolved(type_ref))
    return _handle(DynamicValue, type_ref, stored, False)


def _frozen_datum(value: DynamicValue) -> object:
    """The frozen storage of a typed key or element."""
    if value._frozen:
        return value._datum
    return freeze_key(value._datum, resolved(value._type_ref))


def _probe_key(probe: object, type_ref: TypeRef) -> object:
    """The frozen storage of a lookup probe, which must be a canonical value
    of the parent's declared ``type_ref``."""
    if isinstance(probe, DynamicValue):
        if probe.type_ref != type_ref:
            raise TypeError(
                f"probe of type '{type_name(probe.type_ref)}' does not match "
                f"'{type_name(type_ref)}'"
            )
        return _frozen_datum(probe)
    try:
        return freeze_key(probe, type_ref)
    except EncodeError as e:
        raise TypeError(f"invalid probe: {e}") from e


def _find(
    type_ref: TypeRef, stored: Iterable[object], datum: object
) -> DynamicValue | None:
    # Identity first: an admitted NaN never equals itself.
    for element in stored:
        if element is datum or element == datum:
            return _key_handle(type_ref, element)
    return None


def _field_id(field_id: object) -> int:
    if isinstance(field_id, bool) or not isinstance(field_id, int):
        raise TypeError(f"field id {field_id!r} is not an int")
    return field_id


def _describe(type_ref: object) -> str:
    return type_name(type_ref) if isinstance(type_ref, TypeRef) else repr(type_ref)
