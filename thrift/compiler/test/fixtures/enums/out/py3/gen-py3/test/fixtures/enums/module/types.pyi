#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/enums/src/module.thrift
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
import test.fixtures.enums.module.thrift_types


Metasyntactic = test.fixtures.enums.module.thrift_types.Metasyntactic
MyEnum1 = test.fixtures.enums.module.thrift_types.MyEnum1
MyEnum2 = test.fixtures.enums.module.thrift_types.MyEnum2
MyEnum3 = test.fixtures.enums.module.thrift_types.MyEnum3
MyEnum4 = test.fixtures.enums.module.thrift_types.MyEnum4
MyBitmaskEnum1 = test.fixtures.enums.module.thrift_types.MyBitmaskEnum1
MyBitmaskEnum2 = test.fixtures.enums.module.thrift_types.MyBitmaskEnum2

class SomeStruct(thrift.py3.types.Struct, _typing.Hashable):
    class __fbthrift_IsSet:
        reasonable: bool
        fine: bool
        questionable: bool
        tags: bool
        pass

    reasonable: _typing.Final[Metasyntactic] = ...
    fine: _typing.Final[Metasyntactic] = ...
    questionable: _typing.Final[Metasyntactic] = ...
    tags: _typing.Final[_typing.AbstractSet[int]] = ...

    def __init__(
        self, *,
        reasonable: _typing.Optional[Metasyntactic]=None,
        fine: _typing.Optional[Metasyntactic]=None,
        questionable: _typing.Optional[Metasyntactic]=None,
        tags: _typing.Optional[_typing.AbstractSet[int]]=None
    ) -> None: ...

    def __call__(
        self, *,
        reasonable: _typing.Union[Metasyntactic, None]=None,
        fine: _typing.Union[Metasyntactic, None]=None,
        questionable: _typing.Union[Metasyntactic, None]=None,
        tags: _typing.Union[_typing.AbstractSet[int], None]=None
    ) -> SomeStruct: ...

    def __reduce__(self) -> _typing.Tuple[_typing.Callable, _typing.Tuple[_typing.Type['SomeStruct'], bytes]]: ...
    def __hash__(self) -> int: ...
    def __str__(self) -> str: ...
    def __repr__(self) -> str: ...
    def __lt__(self, other: 'SomeStruct') -> bool: ...
    def __gt__(self, other: 'SomeStruct') -> bool: ...
    def __le__(self, other: 'SomeStruct') -> bool: ...
    def __ge__(self, other: 'SomeStruct') -> bool: ...

    def _to_python(self) -> test.fixtures.enums.module.thrift_types.SomeStruct: ...
    def _to_py3(self) -> SomeStruct: ...
    def _to_py_deprecated(self) -> "module.ttypes.SomeStruct": ...   # type: ignore

class MyStruct(thrift.py3.types.Struct, _typing.Hashable):
    class __fbthrift_IsSet:
        me2_3: bool
        me3_n3: bool
        me1_t1: bool
        me1_t2: bool
        pass

    me2_3: _typing.Final[MyEnum2] = ...
    me3_n3: _typing.Final[MyEnum3] = ...
    me1_t1: _typing.Final[MyEnum1] = ...
    me1_t2: _typing.Final[MyEnum1] = ...

    def __init__(
        self, *,
        me2_3: _typing.Optional[MyEnum2]=None,
        me3_n3: _typing.Optional[MyEnum3]=None,
        me1_t1: _typing.Optional[MyEnum1]=None,
        me1_t2: _typing.Optional[MyEnum1]=None
    ) -> None: ...

    def __call__(
        self, *,
        me2_3: _typing.Union[MyEnum2, None]=None,
        me3_n3: _typing.Union[MyEnum3, None]=None,
        me1_t1: _typing.Union[MyEnum1, None]=None,
        me1_t2: _typing.Union[MyEnum1, None]=None
    ) -> MyStruct: ...

    def __reduce__(self) -> _typing.Tuple[_typing.Callable, _typing.Tuple[_typing.Type['MyStruct'], bytes]]: ...
    def __hash__(self) -> int: ...
    def __str__(self) -> str: ...
    def __repr__(self) -> str: ...
    def __lt__(self, other: 'MyStruct') -> bool: ...
    def __gt__(self, other: 'MyStruct') -> bool: ...
    def __le__(self, other: 'MyStruct') -> bool: ...
    def __ge__(self, other: 'MyStruct') -> bool: ...

    def _to_python(self) -> test.fixtures.enums.module.thrift_types.MyStruct: ...
    def _to_py3(self) -> MyStruct: ...
    def _to_py_deprecated(self) -> "module.ttypes.MyStruct": ...   # type: ignore

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


