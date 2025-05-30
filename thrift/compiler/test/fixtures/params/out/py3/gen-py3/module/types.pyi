#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/params/src/module.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

import enum as _python_std_enum
import folly.iobuf as _fbthrift_iobuf
import thrift.py3.types
import thrift.python.types
import thrift.py3.exceptions
import typing as _typing

import sys
import itertools
import module.thrift_types



_List__i32T = _typing.TypeVar('_List__i32T', bound=_typing.Sequence[int])


class List__i32(_typing.Sequence[int], _typing.Hashable):
    def __init__(self, items: _typing.Optional[_typing.Sequence[int]]=None) -> None: ...
    def __len__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __copy__(self) -> _typing.Sequence[int]: ...
    @_typing.overload
    def __getitem__(self, i: int) -> int: ...
    @_typing.overload
    def __getitem__(self, s: slice) -> _typing.Sequence[int]: ...
    def __add__(self, other: _typing.Sequence[int]) -> 'List__i32': ...
    def __radd__(self, other: _typing.Sequence[int]) -> 'List__i32': ...
    def __reversed__(self) -> _typing.Iterator[int]: ...
    #pyre-ignore[14]: no idea what pyre is on about
    def __iter__(self) -> _typing.Iterator[int]: ...


class Map__i32_List__i32(_typing.Mapping[int, _typing.Sequence[int]], _typing.Hashable):
    def __init__(self, items: _typing.Optional[_typing.Mapping[int, _typing.Sequence[int]]]=None) -> None: ...
    def __len__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __copy__(self) -> _typing.Mapping[int, _typing.Sequence[int]]: ...
    def __getitem__(self, key: int) -> _typing.Sequence[int]: ...
    def __iter__(self) -> _typing.Iterator[int]: ...


class Set__i32(_typing.AbstractSet[int], _typing.Hashable):
    def __init__(self, items: _typing.Optional[_typing.AbstractSet[int]]=None) -> None: ...
    def __len__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __copy__(self) -> _typing.AbstractSet[int]: ...
    def __contains__(self, x: object) -> bool: ...
    def union(self, other: _typing.AbstractSet[int]) -> 'Set__i32': ...
    def intersection(self, other: _typing.AbstractSet[int]) -> 'Set__i32': ...
    def difference(self, other: _typing.AbstractSet[int]) -> 'Set__i32': ...
    def symmetric_difference(self, other: _typing.AbstractSet[int]) -> 'Set__i32': ...
    def issubset(self, other: _typing.AbstractSet[int]) -> bool: ...
    def issuperset(self, other: _typing.AbstractSet[int]) -> bool: ...
    def __iter__(self) -> _typing.Iterator[int]: ...


class Map__i32_Set__i32(_typing.Mapping[int, _typing.AbstractSet[int]], _typing.Hashable):
    def __init__(self, items: _typing.Optional[_typing.Mapping[int, _typing.AbstractSet[int]]]=None) -> None: ...
    def __len__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __copy__(self) -> _typing.Mapping[int, _typing.AbstractSet[int]]: ...
    def __getitem__(self, key: int) -> _typing.AbstractSet[int]: ...
    def __iter__(self) -> _typing.Iterator[int]: ...


class Map__i32_i32(_typing.Mapping[int, int], _typing.Hashable):
    def __init__(self, items: _typing.Optional[_typing.Mapping[int, int]]=None) -> None: ...
    def __len__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __copy__(self) -> _typing.Mapping[int, int]: ...
    def __getitem__(self, key: int) -> int: ...
    def __iter__(self) -> _typing.Iterator[int]: ...


_List__Map__i32_i32T = _typing.TypeVar('_List__Map__i32_i32T', bound=_typing.Sequence[_typing.Mapping[int, int]])


class List__Map__i32_i32(_typing.Sequence[_typing.Mapping[int, int]], _typing.Hashable):
    def __init__(self, items: _typing.Optional[_typing.Sequence[_typing.Mapping[int, int]]]=None) -> None: ...
    def __len__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __copy__(self) -> _typing.Sequence[_typing.Mapping[int, int]]: ...
    @_typing.overload
    def __getitem__(self, i: int) -> _typing.Mapping[int, int]: ...
    @_typing.overload
    def __getitem__(self, s: slice) -> _typing.Sequence[_typing.Mapping[int, int]]: ...
    def __add__(self, other: _typing.Sequence[_typing.Mapping[int, int]]) -> 'List__Map__i32_i32': ...
    def __radd__(self, other: _typing.Sequence[_typing.Mapping[int, int]]) -> 'List__Map__i32_i32': ...
    def __reversed__(self) -> _typing.Iterator[_typing.Mapping[int, int]]: ...
    #pyre-ignore[14]: no idea what pyre is on about
    def __iter__(self) -> _typing.Iterator[_typing.Mapping[int, int]]: ...


_List__Set__i32T = _typing.TypeVar('_List__Set__i32T', bound=_typing.Sequence[_typing.AbstractSet[int]])


class List__Set__i32(_typing.Sequence[_typing.AbstractSet[int]], _typing.Hashable):
    def __init__(self, items: _typing.Optional[_typing.Sequence[_typing.AbstractSet[int]]]=None) -> None: ...
    def __len__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __copy__(self) -> _typing.Sequence[_typing.AbstractSet[int]]: ...
    @_typing.overload
    def __getitem__(self, i: int) -> _typing.AbstractSet[int]: ...
    @_typing.overload
    def __getitem__(self, s: slice) -> _typing.Sequence[_typing.AbstractSet[int]]: ...
    def __add__(self, other: _typing.Sequence[_typing.AbstractSet[int]]) -> 'List__Set__i32': ...
    def __radd__(self, other: _typing.Sequence[_typing.AbstractSet[int]]) -> 'List__Set__i32': ...
    def __reversed__(self) -> _typing.Iterator[_typing.AbstractSet[int]]: ...
    #pyre-ignore[14]: no idea what pyre is on about
    def __iter__(self) -> _typing.Iterator[_typing.AbstractSet[int]]: ...


class Map__i32_Map__i32_Set__i32(_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]], _typing.Hashable):
    def __init__(self, items: _typing.Optional[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]]=None) -> None: ...
    def __len__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __copy__(self) -> _typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]: ...
    def __getitem__(self, key: int) -> _typing.Mapping[int, _typing.AbstractSet[int]]: ...
    def __iter__(self) -> _typing.Iterator[int]: ...


_List__Map__i32_Map__i32_Set__i32T = _typing.TypeVar('_List__Map__i32_Map__i32_Set__i32T', bound=_typing.Sequence[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]])


class List__Map__i32_Map__i32_Set__i32(_typing.Sequence[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]], _typing.Hashable):
    def __init__(self, items: _typing.Optional[_typing.Sequence[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]]]=None) -> None: ...
    def __len__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __copy__(self) -> _typing.Sequence[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]]: ...
    @_typing.overload
    def __getitem__(self, i: int) -> _typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]: ...
    @_typing.overload
    def __getitem__(self, s: slice) -> _typing.Sequence[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]]: ...
    def __add__(self, other: _typing.Sequence[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]]) -> 'List__Map__i32_Map__i32_Set__i32': ...
    def __radd__(self, other: _typing.Sequence[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]]) -> 'List__Map__i32_Map__i32_Set__i32': ...
    def __reversed__(self) -> _typing.Iterator[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]]: ...
    #pyre-ignore[14]: no idea what pyre is on about
    def __iter__(self) -> _typing.Iterator[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]]: ...


_List__List__Map__i32_Map__i32_Set__i32T = _typing.TypeVar('_List__List__Map__i32_Map__i32_Set__i32T', bound=_typing.Sequence[_typing.Sequence[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]]])


class List__List__Map__i32_Map__i32_Set__i32(_typing.Sequence[_typing.Sequence[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]]], _typing.Hashable):
    def __init__(self, items: _typing.Optional[_typing.Sequence[_typing.Sequence[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]]]]=None) -> None: ...
    def __len__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __copy__(self) -> _typing.Sequence[_typing.Sequence[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]]]: ...
    @_typing.overload
    def __getitem__(self, i: int) -> _typing.Sequence[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]]: ...
    @_typing.overload
    def __getitem__(self, s: slice) -> _typing.Sequence[_typing.Sequence[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]]]: ...
    def __add__(self, other: _typing.Sequence[_typing.Sequence[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]]]) -> 'List__List__Map__i32_Map__i32_Set__i32': ...
    def __radd__(self, other: _typing.Sequence[_typing.Sequence[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]]]) -> 'List__List__Map__i32_Map__i32_Set__i32': ...
    def __reversed__(self) -> _typing.Iterator[_typing.Sequence[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]]]: ...
    #pyre-ignore[14]: no idea what pyre is on about
    def __iter__(self) -> _typing.Iterator[_typing.Sequence[_typing.Mapping[int, _typing.Mapping[int, _typing.AbstractSet[int]]]]]: ...


