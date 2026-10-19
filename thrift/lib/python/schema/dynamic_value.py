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
Schema-typed handles on owned datum (see ``_datum``).

``MutableDynamicValue`` owns a datum that is valid for its ``TypeRef`` and
allows checked mutation through mutable views. ``DynamicValue`` is its
read-only base; ``read_only()`` returns a live read-only handle, not a
snapshot. Scalars are read through checked accessors; containers, structs, and
unions through views whose children take their types from the parent's schema.

Handles refer to stored objects, not storage slots, as Python names do. A
scalar read is a value. A struct, container, or union read shares the stored
object, so in-place edits are visible through every handle on it; replacing the
slot leaves earlier handles on the old object. Writes validate and copy the
incoming value. Map keys and set elements are frozen and always read-only.
Presence is canonical: ``UNSET`` stands only for an unset optional struct
field, and every other field holds an explicit value.
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

# pyrefly: ignore [missing-import]
from thrift.lib.python.schema._datum import (
    check_frozen_key,
    check_union_shape,
    copy_value,
    default_value,
    field_default,
    freeze_key,
    FrozenMap,
    resolved,
    true_type,
    type_name,
    UNSET,
    values_equal,
)

# pyrefly: ignore [missing-import]
from thrift.lib.python.schema._errors import EncodeError

# pyrefly: ignore [missing-import]
from thrift.lib.python.schema.type_system import (
    EnumTypeRef,
    FieldDefinition,
    ListTypeRef,
    MapTypeRef,
    PresenceQualifier,
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
    "MutableDynamicValue",
    "MutableListView",
    "MutableMapView",
    "MutableSetView",
    "MutableStructView",
    "MutableUnionView",
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

    Values are constructed as ``MutableDynamicValue``; ``read_only()`` and
    read-only views return this class. The datum may be shared with a mutable
    value, so a read-only handle observes later in-place edits. Equality
    compares the ``TypeRef`` and the current value; a handle never equals a raw
    Python value and is unhashable."""

    __slots__ = ("_type_ref", "_datum", "_frozen")
    _type_ref: TypeRef
    _datum: object
    _frozen: bool

    def __init__(self, *args: object, **kwargs: object) -> None:
        raise TypeError(
            "DynamicValue is not constructed directly; construct a "
            "MutableDynamicValue and call read_only()"
        )

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

    def read_only(self) -> DynamicValue:
        """A read-only handle on the same datum."""
        return _handle(DynamicValue, self._type_ref, self._datum, self._frozen)

    def copy(self) -> MutableDynamicValue:
        """An independent mutable value equal to this one."""
        return _handle(MutableDynamicValue, self._type_ref, self.to_datum(), False)

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


class MutableDynamicValue(DynamicValue):
    """An owned datum and the ``TypeRef`` it was declared as, mutable through
    its views.

    Construction validates the datum recursively and copies it, so the value
    never shares storage with the caller."""

    __slots__ = ()

    def __init__(self, type_ref: TypeRef, datum: object) -> None:
        stored = copy_value(datum, resolved(type_ref))
        object.__setattr__(self, "_type_ref", type_ref)
        object.__setattr__(self, "_datum", stored)
        object.__setattr__(self, "_frozen", False)

    @classmethod
    def default(cls, type_ref: TypeRef) -> Self:
        """The default value of ``type_ref``, with each non-optional struct
        field holding its custom or intrinsic default."""
        return cls(type_ref, default_value(resolved(type_ref)))

    def as_list(self) -> MutableListView:
        target = self._target(ListTypeRef, "a list")
        elements = cast(list[object], self._datum)
        return MutableListView(resolved(target.element_type), elements)

    def as_set(self) -> MutableSetView:
        target = self._target(SetTypeRef, "a set")
        elements = cast(dict[object, None], self._datum)
        return MutableSetView(resolved(target.element_type), elements)

    def as_map(self) -> MutableMapView:
        target = self._target(MapTypeRef, "a map")
        entries = cast(dict[object, object], self._datum)
        key_type, value_type = resolved(target.key_type), resolved(target.value_type)
        return MutableMapView(key_type, value_type, entries)

    def as_struct(self) -> MutableStructView:
        """The present fields of a struct or exception."""
        target = self._target(StructTypeRef, "a struct")
        return MutableStructView(target.node, cast(list[object], self._datum))

    def as_union(self) -> MutableUnionView:
        target = self._target(UnionTypeRef, "a union")
        return MutableUnionView(target, cast(list[object], self._datum))

    def __reduce__(
        self,
    ) -> tuple[Callable[[TypeRef, object], DynamicValue], tuple[TypeRef, object]]:
        return (MutableDynamicValue, (self._type_ref, self.to_datum()))


# ---------------------------------------------------------------------------
# Views. Each holds the parent's stored container and the child types its
# schema declares, and wraps children as they are read. Read-only views wrap
# children as DynamicValue; mutable views as MutableDynamicValue.
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


class MutableListView(ListView[MutableDynamicValue]):
    """The elements of a list value, with checked writes."""

    __slots__ = ()
    _HANDLE = MutableDynamicValue

    def __init__(self, element_type: TypeRef, elements: list[object]) -> None:
        super().__init__(element_type, elements, False)

    @overload
    def __setitem__(self, index: int, value: object) -> None: ...

    @overload
    def __setitem__(self, index: slice, value: Iterable[object]) -> None: ...

    def __setitem__(self, index: int | slice, value: object) -> None:
        if isinstance(index, slice):
            self._list()[index] = self._owned_all(cast(Iterable[object], value))
        else:
            self._list()[index] = _owned_copy(value, self._element_type)

    def __delitem__(self, index: int | slice) -> None:
        del self._list()[index]

    def append(self, value: object) -> None:
        self._list().append(_owned_copy(value, self._element_type))

    def extend(self, values: Iterable[object]) -> None:
        self._list().extend(self._owned_all(values))

    def insert(self, index: int, value: object) -> None:
        self._list().insert(index, _owned_copy(value, self._element_type))

    def clear(self) -> None:
        self._list().clear()

    def _owned_all(self, values: Iterable[object]) -> list[object]:
        element_type = self._element_type
        return [_owned_copy(value, element_type) for value in values]

    def _list(self) -> list[object]:
        return cast(list[object], self._elements)


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


class MutableSetView(SetView):
    """The elements of a set value, with checked writes. ``discard`` and
    ``remove`` take probes, as ``find`` does."""

    __slots__ = ()

    def add(self, value: object) -> None:
        stored = _frozen_copy(value, self._element_type)
        self._dict().setdefault(stored, None)

    def discard(self, probe: object) -> None:
        self._dict().pop(_probe_key(probe, self._element_type), None)

    def remove(self, probe: object) -> None:
        stored = _probe_key(probe, self._element_type)
        elements = self._dict()
        if stored not in elements:
            raise KeyError(probe)
        del elements[stored]

    def clear(self) -> None:
        self._dict().clear()

    def _dict(self) -> dict[object, None]:
        return cast(dict[object, None], self._elements)


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


class MutableMapView(MapView[MutableDynamicValue]):
    """The entries of a map value, with checked writes. Indexing assignment
    and deletion require ``DynamicValue`` keys, as indexing does; ``set`` and
    ``remove`` also accept raw keys, as ``lookup`` does."""

    __slots__ = ()
    _HANDLE = MutableDynamicValue

    def __init__(
        self, key_type: TypeRef, value_type: TypeRef, entries: dict[object, object]
    ) -> None:
        super().__init__(key_type, value_type, entries, False)

    def __setitem__(self, key: DynamicValue, value: object) -> None:
        if not isinstance(key, DynamicValue):
            raise TypeError(
                f"map key {key!r} is not a DynamicValue; use set() for raw datum"
            )
        self.set(key, value)

    def __delitem__(self, key: DynamicValue) -> None:
        if not isinstance(key, DynamicValue):
            raise TypeError(
                f"map key {key!r} is not a DynamicValue; use remove() for raw datum"
            )
        if key.type_ref != self._key_type:
            raise KeyError(key)
        self._pop(_frozen_datum(key), key)

    def set(self, key: object, value: object) -> None:
        """Insert or update the entry for ``key`` (raw datum or a
        ``DynamicValue``). An existing key keeps its position."""
        stored_key = _frozen_copy(key, self._key_type)
        stored_value = _owned_copy(value, self._value_type)
        self._dict()[stored_key] = stored_value

    def remove(self, key: object) -> None:
        """Remove the entry for ``key`` (raw datum or a ``DynamicValue``)."""
        self._pop(_probe_key(key, self._key_type), key)

    def clear(self) -> None:
        self._dict().clear()

    def _pop(self, stored: object, key: object) -> None:
        entries = self._dict()
        if stored not in entries:
            raise KeyError(key)
        del entries[stored]

    def _dict(self) -> dict[object, object]:
        return cast(dict[object, object], self._entries)


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


class MutableStructView(StructView[MutableDynamicValue]):
    """The present fields of a struct or exception value, with checked writes.
    Deleting a field, by ``del`` or ``clear_optional_field``, applies only to
    optional fields."""

    __slots__ = ()
    _HANDLE = MutableDynamicValue

    def __init__(self, node: StructNode, values: list[object]) -> None:
        super().__init__(node, values, False)

    def __setitem__(self, name: str, value: object) -> None:
        self._store(self._by_name(name), value)

    def __delitem__(self, name: str) -> None:
        field = self._by_name(name)
        _require_optional(field)
        index = self._index(field)
        if self._values[index] is UNSET:
            raise KeyError(name)
        self._list()[index] = UNSET

    def set_field(self, selector: str | int, value: object) -> None:
        """Set the field named, or with the id, ``selector``."""
        self._store(self._select(selector), value)

    def clear_optional_field(self, selector: str | int) -> None:
        """Make an optional field absent; clearing an absent field does
        nothing."""
        field = self._select(selector)
        _require_optional(field)
        self._list()[self._index(field)] = UNSET

    def reset_field(self, selector: str | int) -> None:
        """Restore a field's schema default: absent for an optional field."""
        field = self._select(selector)
        self._list()[self._index(field)] = field_default(field)

    def _select(self, selector: str | int) -> FieldDefinition:
        if isinstance(selector, str):
            return self._by_name(selector)
        return self._by_id(selector)

    def _store(self, field: FieldDefinition, value: object) -> None:
        stored = _owned_copy(value, field.type)
        self._list()[self._index(field)] = stored

    def _list(self) -> list[object]:
        return cast(list[object], self._values)


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


class MutableUnionView(UnionView[MutableDynamicValue]):
    """The active field of a union value, with checked writes."""

    __slots__ = ()
    _HANDLE = MutableDynamicValue

    def __init__(self, union_type: UnionTypeRef, stored: list[object]) -> None:
        super().__init__(union_type, stored, False)

    def set_field(self, selector: str | int, value: object) -> None:
        """Make the field named, or with the id, ``selector`` active."""
        field = self._field(selector)
        stored = _owned_copy(value, field.type)
        cast(list[object], self._datum)[:] = [field.identity.id, stored]

    def clear(self) -> None:
        """Make the union empty."""
        cast(list[object], self._datum).clear()


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
    return MutableDynamicValue(type_ref, datum).read_only()


def _check_type(value: DynamicValue, type_ref: TypeRef) -> None:
    if value.type_ref != type_ref:
        raise TypeError(
            f"value of type '{type_name(value.type_ref)}' does not match "
            f"'{type_name(type_ref)}'"
        )


def _owned_copy(value: object, type_ref: TypeRef) -> object:
    """Owned storage for an incoming value of ``type_ref``: raw datum, or a
    ``DynamicValue`` with that exact ``TypeRef``."""
    if isinstance(value, DynamicValue):
        _check_type(value, type_ref)
        return copy_value(value._datum, type_ref, frozen=value._frozen)
    return copy_value(value, type_ref)


def _frozen_datum(value: DynamicValue) -> object:
    """The frozen storage of a typed key or element."""
    if value._frozen:
        return value._datum
    return freeze_key(value._datum, resolved(value._type_ref))


def _frozen_copy(value: object, type_ref: TypeRef) -> object:
    """Frozen storage for an incoming key or element of ``type_ref``. A typed
    value is validated against ``type_ref`` itself, since ``TypeRef`` equality
    compares user-defined types by URI only."""
    if isinstance(value, DynamicValue):
        _check_type(value, type_ref)
        if value._frozen:
            check_frozen_key(value._datum, type_ref)
            return value._datum
        return freeze_key(value._datum, type_ref)
    return freeze_key(value, type_ref)


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


def _require_optional(field: FieldDefinition) -> None:
    if field.presence is not PresenceQualifier.OPTIONAL:
        raise TypeError(f"field '{field.identity.name}' is not optional")


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
