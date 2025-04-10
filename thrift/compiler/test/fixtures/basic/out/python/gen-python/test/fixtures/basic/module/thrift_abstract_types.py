

#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from __future__ import annotations

import abc as _abc
import typing as _typing
import builtins as _fbthrift_builtins


import enum as _enum


import folly.iobuf as _fbthrift_iobuf
import thrift.python.abstract_types as _fbthrift_python_abstract_types

from test.fixtures.basic.module.thrift_enums import (
    MyEnum,
    MyEnum as _fbthrift_MyEnum,
    HackEnum,
    HackEnum as _fbthrift_HackEnum,
)

class MyStruct(_abc.ABC):
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def MyIntField(self) -> int: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def MyStringField(self) -> str: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def MyDataField(self) -> _fbthrift_MyDataItem: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def myEnum(self) -> _fbthrift_MyEnum: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def oneway(self) -> bool: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def readonly(self) -> bool: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def idempotent(self) -> bool: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def floatSet(self) -> _typing.AbstractSet[float]: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def no_hack_codegen_field(self) -> str: ...
    @_abc.abstractmethod
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int, str, _fbthrift_MyDataItem, _fbthrift_MyEnum, bool, bool, bool, _typing.AbstractSet[float], str]]]: ...
    @_abc.abstractmethod
    def _to_mutable_python(self) -> "test.fixtures.basic.module.thrift_mutable_types.MyStruct": ...  # type: ignore
    @_abc.abstractmethod
    def _to_python(self) -> "test.fixtures.basic.module.thrift_types.MyStruct": ...  # type: ignore
    @_abc.abstractmethod
    def _to_py3(self) -> "test.fixtures.basic.module.types.MyStruct": ...  # type: ignore
    @_abc.abstractmethod
    def _to_py_deprecated(self) -> "module.ttypes.MyStruct": ...  # type: ignore
_fbthrift_MyStruct = MyStruct
class Containers(_abc.ABC):
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def I32List(self) -> _typing.Sequence[int]: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def StringSet(self) -> _typing.AbstractSet[str]: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def StringToI64Map(self) -> _typing.Mapping[str, int]: ...
    @_abc.abstractmethod
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[_typing.Sequence[int], _typing.AbstractSet[str], _typing.Mapping[str, int]]]]: ...
    @_abc.abstractmethod
    def _to_mutable_python(self) -> "test.fixtures.basic.module.thrift_mutable_types.Containers": ...  # type: ignore
    @_abc.abstractmethod
    def _to_python(self) -> "test.fixtures.basic.module.thrift_types.Containers": ...  # type: ignore
    @_abc.abstractmethod
    def _to_py3(self) -> "test.fixtures.basic.module.types.Containers": ...  # type: ignore
    @_abc.abstractmethod
    def _to_py_deprecated(self) -> "module.ttypes.Containers": ...  # type: ignore
_fbthrift_Containers = Containers
class MyDataItem(_abc.ABC):
    @_abc.abstractmethod
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[None]]]: ...
    @_abc.abstractmethod
    def _to_mutable_python(self) -> "test.fixtures.basic.module.thrift_mutable_types.MyDataItem": ...  # type: ignore
    @_abc.abstractmethod
    def _to_python(self) -> "test.fixtures.basic.module.thrift_types.MyDataItem": ...  # type: ignore
    @_abc.abstractmethod
    def _to_py3(self) -> "test.fixtures.basic.module.types.MyDataItem": ...  # type: ignore
    @_abc.abstractmethod
    def _to_py_deprecated(self) -> "module.ttypes.MyDataItem": ...  # type: ignore
_fbthrift_MyDataItem = MyDataItem
class MyUnion(_abc.ABC):
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def myEnum(self) -> _fbthrift_MyEnum: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def myStruct(self) -> _fbthrift_MyStruct: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def myDataItem(self) -> _fbthrift_MyDataItem: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def floatSet(self) -> _typing.AbstractSet[float]: ...
    @_abc.abstractmethod
    def _to_mutable_python(self) -> "test.fixtures.basic.module.thrift_mutable_types.MyUnion": ...  # type: ignore
    @_abc.abstractmethod
    def _to_python(self) -> "test.fixtures.basic.module.thrift_types.MyUnion": ...  # type: ignore
    @_abc.abstractmethod
    def _to_py3(self) -> "test.fixtures.basic.module.types.MyUnion": ...  # type: ignore
    @_abc.abstractmethod
    def _to_py_deprecated(self) -> "module.ttypes.MyUnion": ...  # type: ignore

    class FbThriftUnionFieldEnum(_enum.Enum):
        EMPTY = 0
        myEnum = 1
        myStruct = 2
        myDataItem = 3
        floatSet = 4

    FbThriftUnionFieldEnum.__name__ = "MyUnion"
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def fbthrift_current_value(self) -> _typing.Union[None, _fbthrift_MyEnum, _fbthrift_MyStruct, _fbthrift_MyDataItem, _typing.AbstractSet[float]]: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def fbthrift_current_field(self) -> FbThriftUnionFieldEnum: ...

_fbthrift_MyUnion = MyUnion
class MyException(_fbthrift_python_abstract_types.AbstractGeneratedError):
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    def MyIntField(self) -> int: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    def MyStringField(self) -> str: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    def myStruct(self) -> _fbthrift_MyStruct: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    def myUnion(self) -> _fbthrift_MyUnion: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int, str, _fbthrift_MyStruct, _fbthrift_MyUnion]]]: ...
    def _to_mutable_python(self) -> "test.fixtures.basic.module.thrift_mutable_types.MyException": ...  # type: ignore
    def _to_python(self) -> "test.fixtures.basic.module.thrift_types.MyException": ...  # type: ignore
    def _to_py3(self) -> "test.fixtures.basic.module.types.MyException": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.MyException": ...  # type: ignore
_fbthrift_MyException = MyException
class MyExceptionWithMessage(_fbthrift_python_abstract_types.AbstractGeneratedError):
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    def MyIntField(self) -> int: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    def MyStringField(self) -> str: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    def myStruct(self) -> _fbthrift_MyStruct: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    def myUnion(self) -> _fbthrift_MyUnion: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int, str, _fbthrift_MyStruct, _fbthrift_MyUnion]]]: ...
    def _to_mutable_python(self) -> "test.fixtures.basic.module.thrift_mutable_types.MyExceptionWithMessage": ...  # type: ignore
    def _to_python(self) -> "test.fixtures.basic.module.thrift_types.MyExceptionWithMessage": ...  # type: ignore
    def _to_py3(self) -> "test.fixtures.basic.module.types.MyExceptionWithMessage": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.MyExceptionWithMessage": ...  # type: ignore
_fbthrift_MyExceptionWithMessage = MyExceptionWithMessage
class ReservedKeyword(_abc.ABC):
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def reserved_field(self) -> int: ...
    @_abc.abstractmethod
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int]]]: ...
    @_abc.abstractmethod
    def _to_mutable_python(self) -> "test.fixtures.basic.module.thrift_mutable_types.ReservedKeyword": ...  # type: ignore
    @_abc.abstractmethod
    def _to_python(self) -> "test.fixtures.basic.module.thrift_types.ReservedKeyword": ...  # type: ignore
    @_abc.abstractmethod
    def _to_py3(self) -> "test.fixtures.basic.module.types.ReservedKeyword": ...  # type: ignore
    @_abc.abstractmethod
    def _to_py_deprecated(self) -> "module.ttypes.ReservedKeyword": ...  # type: ignore
_fbthrift_ReservedKeyword = ReservedKeyword
class UnionToBeRenamed(_abc.ABC):
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def reserved_field(self) -> int: ...
    @_abc.abstractmethod
    def _to_mutable_python(self) -> "test.fixtures.basic.module.thrift_mutable_types.UnionToBeRenamed": ...  # type: ignore
    @_abc.abstractmethod
    def _to_python(self) -> "test.fixtures.basic.module.thrift_types.UnionToBeRenamed": ...  # type: ignore
    @_abc.abstractmethod
    def _to_py3(self) -> "test.fixtures.basic.module.types.UnionToBeRenamed": ...  # type: ignore
    @_abc.abstractmethod
    def _to_py_deprecated(self) -> "module.ttypes.UnionToBeRenamed": ...  # type: ignore

    class FbThriftUnionFieldEnum(_enum.Enum):
        EMPTY = 0
        reserved_field = 1

    FbThriftUnionFieldEnum.__name__ = "UnionToBeRenamed"
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def fbthrift_current_value(self) -> _typing.Union[None, int]: ...
    # pyre-ignore[16]: Module `_fbthrift_builtins` has no attribute `property`.
    @_fbthrift_builtins.property
    @_abc.abstractmethod
    def fbthrift_current_field(self) -> FbThriftUnionFieldEnum: ...

_fbthrift_UnionToBeRenamed = UnionToBeRenamed

MyEnumAlias = _fbthrift_MyEnum
MyDataItemAlias = _fbthrift_MyDataItem
