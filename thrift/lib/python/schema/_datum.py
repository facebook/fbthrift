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
