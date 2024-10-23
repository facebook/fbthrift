#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from __future__ import annotations

import typing as _typing

import folly.iobuf as _fbthrift_iobuf
import test.fixtures.enums.module.thrift_abstract_types
import thrift.python.types as _fbthrift_python_types
import thrift.python.exceptions as _fbthrift_python_exceptions

class _fbthrift_compatible_with_Metasyntactic:
    pass


class Metasyntactic(_fbthrift_python_types.Enum, int, test.fixtures.enums.module.thrift_abstract_types.Metasyntactic, _fbthrift_compatible_with_Metasyntactic):
    FOO: Metasyntactic = ...
    BAR: Metasyntactic = ...
    BAZ: Metasyntactic = ...
    BAX: Metasyntactic = ...
    def _to_python(self) -> Metasyntactic: ...
    def _to_py3(self) -> "test.fixtures.enums.module.types.Metasyntactic": ...  # type: ignore
    def _to_py_deprecated(self) -> int: ...

class _fbthrift_compatible_with_MyEnum1:
    pass


class MyEnum1(_fbthrift_python_types.Enum, int, test.fixtures.enums.module.thrift_abstract_types.MyEnum1, _fbthrift_compatible_with_MyEnum1):
    ME1_0: MyEnum1 = ...
    ME1_1: MyEnum1 = ...
    ME1_2: MyEnum1 = ...
    ME1_3: MyEnum1 = ...
    ME1_5: MyEnum1 = ...
    ME1_6: MyEnum1 = ...
    def _to_python(self) -> MyEnum1: ...
    def _to_py3(self) -> "test.fixtures.enums.module.types.MyEnum1": ...  # type: ignore
    def _to_py_deprecated(self) -> int: ...

class _fbthrift_compatible_with_MyEnum2:
    pass


class MyEnum2(_fbthrift_python_types.Enum, int, test.fixtures.enums.module.thrift_abstract_types.MyEnum2, _fbthrift_compatible_with_MyEnum2):
    ME2_0: MyEnum2 = ...
    ME2_1: MyEnum2 = ...
    ME2_2: MyEnum2 = ...
    def _to_python(self) -> MyEnum2: ...
    def _to_py3(self) -> "test.fixtures.enums.module.types.MyEnum2": ...  # type: ignore
    def _to_py_deprecated(self) -> int: ...

class _fbthrift_compatible_with_MyEnum3:
    pass


class MyEnum3(_fbthrift_python_types.Enum, int, test.fixtures.enums.module.thrift_abstract_types.MyEnum3, _fbthrift_compatible_with_MyEnum3):
    ME3_0: MyEnum3 = ...
    ME3_1: MyEnum3 = ...
    ME3_N2: MyEnum3 = ...
    ME3_N1: MyEnum3 = ...
    ME3_9: MyEnum3 = ...
    ME3_10: MyEnum3 = ...
    def _to_python(self) -> MyEnum3: ...
    def _to_py3(self) -> "test.fixtures.enums.module.types.MyEnum3": ...  # type: ignore
    def _to_py_deprecated(self) -> int: ...

class _fbthrift_compatible_with_MyEnum4:
    pass


class MyEnum4(_fbthrift_python_types.Enum, int, test.fixtures.enums.module.thrift_abstract_types.MyEnum4, _fbthrift_compatible_with_MyEnum4):
    ME4_A: MyEnum4 = ...
    ME4_B: MyEnum4 = ...
    ME4_C: MyEnum4 = ...
    ME4_D: MyEnum4 = ...
    def _to_python(self) -> MyEnum4: ...
    def _to_py3(self) -> "test.fixtures.enums.module.types.MyEnum4": ...  # type: ignore
    def _to_py_deprecated(self) -> int: ...

class _fbthrift_compatible_with_MyBitmaskEnum1:
    pass


class MyBitmaskEnum1(_fbthrift_python_types.Enum, int, test.fixtures.enums.module.thrift_abstract_types.MyBitmaskEnum1, _fbthrift_compatible_with_MyBitmaskEnum1):
    ONE: MyBitmaskEnum1 = ...
    TWO: MyBitmaskEnum1 = ...
    FOUR: MyBitmaskEnum1 = ...
    def _to_python(self) -> MyBitmaskEnum1: ...
    def _to_py3(self) -> "test.fixtures.enums.module.types.MyBitmaskEnum1": ...  # type: ignore
    def _to_py_deprecated(self) -> int: ...

class _fbthrift_compatible_with_MyBitmaskEnum2:
    pass


class MyBitmaskEnum2(_fbthrift_python_types.Enum, int, test.fixtures.enums.module.thrift_abstract_types.MyBitmaskEnum2, _fbthrift_compatible_with_MyBitmaskEnum2):
    ONE: MyBitmaskEnum2 = ...
    TWO: MyBitmaskEnum2 = ...
    FOUR: MyBitmaskEnum2 = ...
    def _to_python(self) -> MyBitmaskEnum2: ...
    def _to_py3(self) -> "test.fixtures.enums.module.types.MyBitmaskEnum2": ...  # type: ignore
    def _to_py_deprecated(self) -> int: ...


class _fbthrift_compatible_with_SomeStruct:
    pass


class SomeStruct(_fbthrift_python_types.Struct, _fbthrift_compatible_with_SomeStruct, test.fixtures.enums.module.thrift_abstract_types.SomeStruct):
    reasonable: _typing.Final[Metasyntactic] = ...
    fine: _typing.Final[Metasyntactic] = ...
    questionable: _typing.Final[Metasyntactic] = ...
    tags: _typing.Final[_typing.AbstractSet[int]] = ...
    def __init__(
        self, *,
        reasonable: _typing.Optional[_fbthrift_compatible_with_Metasyntactic]=...,
        fine: _typing.Optional[_fbthrift_compatible_with_Metasyntactic]=...,
        questionable: _typing.Optional[_fbthrift_compatible_with_Metasyntactic]=...,
        tags: _typing.Optional[_typing.AbstractSet[int]]=...
    ) -> None: ...

    def __call__(
        self, *,
        reasonable: _typing.Optional[_fbthrift_compatible_with_Metasyntactic]=...,
        fine: _typing.Optional[_fbthrift_compatible_with_Metasyntactic]=...,
        questionable: _typing.Optional[_fbthrift_compatible_with_Metasyntactic]=...,
        tags: _typing.Optional[_typing.AbstractSet[int]]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[Metasyntactic, Metasyntactic, Metasyntactic, _typing.AbstractSet[int]]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "test.fixtures.enums.module.types.SomeStruct": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.SomeStruct": ...  # type: ignore


class _fbthrift_compatible_with_MyStruct:
    pass


class MyStruct(_fbthrift_python_types.Struct, _fbthrift_compatible_with_MyStruct, test.fixtures.enums.module.thrift_abstract_types.MyStruct):
    me2_3: _typing.Final[MyEnum2] = ...
    me3_n3: _typing.Final[MyEnum3] = ...
    me1_t1: _typing.Final[MyEnum1] = ...
    me1_t2: _typing.Final[MyEnum1] = ...
    def __init__(
        self, *,
        me2_3: _typing.Optional[_fbthrift_compatible_with_MyEnum2]=...,
        me3_n3: _typing.Optional[_fbthrift_compatible_with_MyEnum3]=...,
        me1_t1: _typing.Optional[_fbthrift_compatible_with_MyEnum1]=...,
        me1_t2: _typing.Optional[_fbthrift_compatible_with_MyEnum1]=...
    ) -> None: ...

    def __call__(
        self, *,
        me2_3: _typing.Optional[_fbthrift_compatible_with_MyEnum2]=...,
        me3_n3: _typing.Optional[_fbthrift_compatible_with_MyEnum3]=...,
        me1_t1: _typing.Optional[_fbthrift_compatible_with_MyEnum1]=...,
        me1_t2: _typing.Optional[_fbthrift_compatible_with_MyEnum1]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[MyEnum2, MyEnum3, MyEnum1, MyEnum1]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "test.fixtures.enums.module.types.MyStruct": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.MyStruct": ...  # type: ignore
