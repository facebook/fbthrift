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

# pyre-strict

import typing
from collections.abc import (
    Iterator,
    MutableMapping as MutableMappingAbc,
    MutableSequence,
    MutableSet as MutableSetAbc,
)
from typing import (
    Any,
    Generic,
    Iterable,
    List,
    Mapping,
    Optional,
    overload,
    Tuple,
    TypeVar,
    Union,
)

from thrift.python.mutable_types import (
    _ThriftListWrapper,
    _ThriftMapWrapper,
    _ThriftSetWrapper,
)

class MapKwargsSentinelType: ...

T = TypeVar("T")

class MutableList(MutableSequence[T]):
    def __init__(
        self,
        typeinfo: object,
        list_data: List[T],
    ) -> None: ...
    def __len__(self) -> int: ...
    @overload
    def __getitem__(self, index: int) -> T: ...
    @overload
    # pyre-fixme[24]: Generic type `slice` expects 3 type parameters.
    def __getitem__(self, index: slice) -> MutableList[T]: ...
    @overload
    def __setitem__(self, index: int, value: T) -> None: ...
    @overload
    def __setitem__(
        self: MutableList[MutableList[Any]],
        index: int,
        value: T | _ThriftListWrapper,
    ) -> None: ...
    @overload
    def __setitem__(
        self: MutableList[MutableSet[Any]], index: int, value: T | _ThriftSetWrapper
    ) -> None: ...
    @overload
    def __setitem__(
        self: MutableList[MutableMap[Any, Any]],
        index: int,
        value: T | _ThriftMapWrapper,
    ) -> None: ...
    @overload
    def __delitem__(self, index: int) -> None: ...
    @overload
    # pyre-fixme[24]: Generic type `slice` expects 3 type parameters.
    def __delitem__(self, index: slice) -> None: ...
    @overload
    def insert(self, index: int, value: T) -> None: ...
    @overload
    def insert(
        self: MutableList[MutableList[Any]],
        index: int,
        value: T | _ThriftListWrapper,
    ) -> None: ...
    @overload
    def insert(
        self: MutableList[MutableSet[Any]], index: int, value: T | _ThriftSetWrapper
    ) -> None: ...
    @overload
    def insert(
        self: MutableList[MutableMap[Any, Any]],
        index: int,
        value: T | _ThriftMapWrapper,
    ) -> None: ...
    @overload
    def append(self, value: T) -> None: ...
    @overload
    def append(
        self: MutableList[MutableList[Any]], value: T | _ThriftListWrapper
    ) -> None: ...
    @overload
    def append(
        self: MutableList[MutableSet[Any]], value: T | _ThriftSetWrapper
    ) -> None: ...
    @overload
    def append(
        self: MutableList[MutableMap[Any, Any]], value: T | _ThriftMapWrapper
    ) -> None: ...
    @overload
    def extend(self, values: Iterable[T]) -> None: ...
    @overload
    def extend(
        self: MutableList[MutableList[Any]],
        values: Iterable[T | _ThriftListWrapper],
    ) -> None: ...
    @overload
    def extend(
        self: MutableList[MutableSet[Any]],
        values: Iterable[T | _ThriftSetWrapper],
    ) -> None: ...
    @overload
    def extend(
        self: MutableList[MutableMap[Any, Any]],
        values: Iterable[T | _ThriftMapWrapper],
    ) -> None: ...
    def pop(self, index: int = -1) -> T: ...
    def clear(self) -> None: ...
    @overload
    def __add__(self, other: Iterable[T]) -> MutableList[T]: ...
    @overload
    def __add__(
        self: MutableList[MutableList[Any]], other: Iterable[T | _ThriftListWrapper]
    ) -> MutableList[T]: ...
    @overload
    def __add__(
        self: MutableList[MutableSet[Any]], other: Iterable[T | _ThriftSetWrapper]
    ) -> MutableList[T]: ...
    @overload
    def __add__(
        self: MutableList[MutableMap[Any, Any]],
        other: Iterable[T | _ThriftMapWrapper],
    ) -> MutableList[T]: ...
    def count(self, value: object) -> int: ...
    def index(
        self,
        value: object,
        start: Optional[int | typing.SupportsIndex] = 0,
        stop: Optional[int | typing.SupportsIndex] = None,
    ) -> int: ...
    def __contains__(self, value: object) -> bool: ...

class MutableSet(MutableSetAbc[T]):
    def __init__(
        self,
        typeinfo: object,
        set_data: typing.Set[object],
    ) -> None: ...
    def __len__(self) -> int: ...
    def __contains__(self, x: object) -> bool: ...
    def __iter__(self) -> ValueIterator[T]: ...
    def isdisjoint(self, other: typing.Iterable[object]) -> bool: ...
    def __le__(self, other: typing.Iterable[object]) -> bool: ...
    def __lt__(self, other: typing.Iterable[object]) -> bool: ...
    def __ge__(self, other: typing.Iterable[object]) -> bool: ...
    def __gt__(self, other: typing.Iterable[object]) -> bool: ...
    def __and__(self, other: typing.Iterable[object]) -> MutableSet[T]: ...
    # pyre-ignore[15]: Inconsistent override
    def __or__(self, other: typing.Iterable[object]) -> MutableSet[T]: ...
    def __sub__(self, other: typing.Iterable[object]) -> MutableSet[T]: ...
    def __xor__(self, other: typing.Iterable[object]) -> MutableSet[T]: ...
    def add(self, value: T) -> None: ...
    def discard(self, value: T) -> None: ...
    def remove(self, value: T) -> None: ...
    def pop(self) -> T: ...
    def clear(self) -> None: ...
    @classmethod
    def _from_iterable(
        cls,
        typeinfo: object,
        set_data: typing.Set[object],
        it: typing.Iterable[object],
    ) -> MutableSet[T]: ...

class ValueIterator(Iterator[T]):
    def __init__(
        self,
        typeinfo: object,
        set_data: typing.Set[object],
    ) -> None: ...
    def __next__(self) -> T: ...
    def __iter__(self) -> ValueIterator[T]: ...

K = TypeVar("K")
V = TypeVar("V")
_K = TypeVar("_K")
_V = TypeVar("_V")

class MutableMap(MutableMappingAbc[K, V]):
    def __init__(
        self,
        key_typeinfo: object,
        val_typeinfo: object,
        map_data: Mapping[object, object],
    ) -> None: ...
    def __len__(self) -> int: ...
    def __getitem__(self, key: K) -> V: ...
    def __iter__(self) -> ValueIterator[K]: ...
    # pyre-ignore[14]: Inconsistent override
    def get(self, key: K, default: Optional[V] = None) -> V: ...
    @overload
    def __setitem__(self, key: K, value: V) -> None: ...
    @overload
    def __setitem__(
        self: MutableMap[K, MutableList[_V]],
        key: K,
        value: MutableList[_V] | _ThriftListWrapper,
    ) -> None: ...
    @overload
    def __setitem__(
        self: MutableMap[K, MutableSet[_V]],
        key: K,
        value: MutableSet[_V] | _ThriftSetWrapper,
    ) -> None: ...
    @overload
    def __setitem__(
        self: MutableMap[K, MutableMap[_K, _V]],
        key: K,
        value: MutableMap[_K, _V] | _ThriftMapWrapper,
    ) -> None: ...
    def __delitem__(self, key: K) -> None: ...
    def __contains__(self, key: T) -> bool: ...
    # pyre-ignore[14]: Inconsistent override
    def update(
        self,
        other: Union[
            Iterable[Tuple[object, object]],
            Mapping[object, object],
        ] = (),
        /,
        **keywords: object,
    ) -> None: ...
    # pyre-ignore[14]: Inconsistent override
    def pop(self, key: T, default: Optional[object] = MapKwargsSentinelType()) -> V: ...
    def popitem(self) -> Tuple[K, V]: ...
    def clear(self) -> None: ...
    # pyre-ignore[15]: Inconsistent override
    def keys(self) -> MapKeysView[K]: ...
    # pyre-ignore[15]: Inconsistent override
    def items(self) -> MapItemsView[K, V]: ...
    # pyre-ignore[15]: Inconsistent override
    def values(self) -> MapValuesView[V]: ...
    # pyre-ignore[14]: Inconsistent override
    def setdefault(self, key: object, default: typing.Optional[object] = None) -> V: ...

class MapKeysView(Generic[K]):
    def __len__(self) -> int: ...
    def __contains__(self, key: object) -> bool: ...
    def __iter__(self) -> ValueIterator[K]: ...

class MapItemsView(Generic[K, V]):
    def __len__(self) -> int: ...
    def __contains__(self, key: K) -> bool: ...
    def __iter__(self) -> MapItemIterator[tuple[K, V]]: ...

class MapItemIterator(Iterator[T]):
    def __next__(self) -> T: ...
    def __iter__(self) -> MapItemIterator[T]: ...

class MapValuesView(Generic[V]):
    def __len__(self) -> int: ...
    def __iter__(self) -> ValueIterator[V]: ...
