#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from __future__ import annotations

import typing as _typing

import enum

import folly.iobuf as _fbthrift_iobuf
import test.fixtures.basic.module.thrift_abstract_types
import thrift.python.types as _fbthrift_python_types
import thrift.python.exceptions as _fbthrift_python_exceptions

class _fbthrift_compatible_with_MyEnum:
    pass


class MyEnum(_fbthrift_python_types.Enum, int, test.fixtures.basic.module.thrift_abstract_types.MyEnum, _fbthrift_compatible_with_MyEnum):
    MyValue1: MyEnum = ...
    MyValue2: MyEnum = ...
    def _to_python(self) -> MyEnum: ...
    def _to_py3(self) -> "test.fixtures.basic.module.types.MyEnum": ...  # type: ignore
    def _to_py_deprecated(self) -> int: ...

class _fbthrift_compatible_with_HackEnum:
    pass


class HackEnum(_fbthrift_python_types.Enum, int, test.fixtures.basic.module.thrift_abstract_types.HackEnum, _fbthrift_compatible_with_HackEnum):
    Value1: HackEnum = ...
    Value2: HackEnum = ...
    def _to_python(self) -> HackEnum: ...
    def _to_py3(self) -> "test.fixtures.basic.module.types.HackEnum": ...  # type: ignore
    def _to_py_deprecated(self) -> int: ...


class _fbthrift_compatible_with_MyStruct:
    pass


class MyStruct(_fbthrift_python_types.Struct, _fbthrift_compatible_with_MyStruct, test.fixtures.basic.module.thrift_abstract_types.MyStruct):
    MyIntField: _typing.Final[int] = ...
    MyStringField: _typing.Final[str] = ...
    MyDataField: _typing.Final[MyDataItem] = ...
    myEnum: _typing.Final[MyEnum] = ...
    oneway: _typing.Final[bool] = ...
    readonly: _typing.Final[bool] = ...
    idempotent: _typing.Final[bool] = ...
    floatSet: _typing.Final[_typing.AbstractSet[float]] = ...
    no_hack_codegen_field: _typing.Final[str] = ...
    def __init__(
        self, *,
        MyIntField: _typing.Optional[int]=...,
        MyStringField: _typing.Optional[str]=...,
        MyDataField: _typing.Optional[_fbthrift_compatible_with_MyDataItem]=...,
        myEnum: _typing.Optional[_fbthrift_compatible_with_MyEnum]=...,
        oneway: _typing.Optional[bool]=...,
        readonly: _typing.Optional[bool]=...,
        idempotent: _typing.Optional[bool]=...,
        floatSet: _typing.Optional[_typing.AbstractSet[float]]=...,
        no_hack_codegen_field: _typing.Optional[str]=...
    ) -> None: ...

    def __call__(
        self, *,
        MyIntField: _typing.Optional[int]=...,
        MyStringField: _typing.Optional[str]=...,
        MyDataField: _typing.Optional[_fbthrift_compatible_with_MyDataItem]=...,
        myEnum: _typing.Optional[_fbthrift_compatible_with_MyEnum]=...,
        oneway: _typing.Optional[bool]=...,
        readonly: _typing.Optional[bool]=...,
        idempotent: _typing.Optional[bool]=...,
        floatSet: _typing.Optional[_typing.AbstractSet[float]]=...,
        no_hack_codegen_field: _typing.Optional[str]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int, str, MyDataItem, MyEnum, bool, bool, bool, _typing.AbstractSet[float], str]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "test.fixtures.basic.module.types.MyStruct": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.MyStruct": ...  # type: ignore


class _fbthrift_compatible_with_Containers:
    pass


class Containers(_fbthrift_python_types.Struct, _fbthrift_compatible_with_Containers, test.fixtures.basic.module.thrift_abstract_types.Containers):
    I32List: _typing.Final[_typing.Sequence[int]] = ...
    StringSet: _typing.Final[_typing.AbstractSet[str]] = ...
    StringToI64Map: _typing.Final[_typing.Mapping[str, int]] = ...
    def __init__(
        self, *,
        I32List: _typing.Optional[_typing.Sequence[int]]=...,
        StringSet: _typing.Optional[_typing.AbstractSet[str]]=...,
        StringToI64Map: _typing.Optional[_typing.Mapping[str, int]]=...
    ) -> None: ...

    def __call__(
        self, *,
        I32List: _typing.Optional[_typing.Sequence[int]]=...,
        StringSet: _typing.Optional[_typing.AbstractSet[str]]=...,
        StringToI64Map: _typing.Optional[_typing.Mapping[str, int]]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[_typing.Sequence[int], _typing.AbstractSet[str], _typing.Mapping[str, int]]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "test.fixtures.basic.module.types.Containers": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.Containers": ...  # type: ignore


class _fbthrift_compatible_with_MyDataItem:
    pass


class MyDataItem(_fbthrift_python_types.Struct, _fbthrift_compatible_with_MyDataItem, test.fixtures.basic.module.thrift_abstract_types.MyDataItem):
    def __init__(
        self,
    ) -> None: ...

    def __call__(
        self,
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[None]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "test.fixtures.basic.module.types.MyDataItem": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.MyDataItem": ...  # type: ignore


class _fbthrift_compatible_with_MyUnion:
    pass


class MyUnion(_fbthrift_python_types.Union, _fbthrift_compatible_with_MyUnion, test.fixtures.basic.module.thrift_abstract_types.MyUnion):
    myEnum: _typing.Final[MyEnum] = ...
    myStruct: _typing.Final[MyStruct] = ...
    myDataItem: _typing.Final[MyDataItem] = ...
    floatSet: _typing.Final[_typing.AbstractSet[float]] = ...
    def __init__(
        self, *,
        myEnum: _typing.Optional[_fbthrift_compatible_with_MyEnum]=...,
        myStruct: _typing.Optional[_fbthrift_compatible_with_MyStruct]=...,
        myDataItem: _typing.Optional[_fbthrift_compatible_with_MyDataItem]=...,
        floatSet: _typing.Optional[_typing.AbstractSet[float]]=...
    ) -> None: ...


    class Type(enum.Enum):
        EMPTY: MyUnion.Type = ...
        myEnum: MyUnion.Type = ...
        myStruct: MyUnion.Type = ...
        myDataItem: MyUnion.Type = ...
        floatSet: MyUnion.Type = ...

    @classmethod
    def fromValue(cls, value: _typing.Union[None, MyEnum, MyStruct, MyDataItem, _typing.AbstractSet[float]]) -> MyUnion: ...
    value: _typing.Final[_typing.Union[None, MyEnum, MyStruct, MyDataItem, _typing.AbstractSet[float]]]
    type: _typing.Final[Type]
    def get_type(self) -> Type:...
    def _to_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "test.fixtures.basic.module.types.MyUnion": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.MyUnion": ...  # type: ignore


class _fbthrift_compatible_with_MyException:
    pass


class MyException(_fbthrift_python_exceptions.GeneratedError, _fbthrift_compatible_with_MyException, test.fixtures.basic.module.thrift_abstract_types.MyException):
    MyIntField: _typing.Final[int] = ...
    MyStringField: _typing.Final[str] = ...
    myStruct: _typing.Final[MyStruct] = ...
    myUnion: _typing.Final[MyUnion] = ...
    def __init__(
        self, *,
        MyIntField: _typing.Optional[int]=...,
        MyStringField: _typing.Optional[str]=...,
        myStruct: _typing.Optional[_fbthrift_compatible_with_MyStruct]=...,
        myUnion: _typing.Optional[_fbthrift_compatible_with_MyUnion]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int, str, MyStruct, MyUnion]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "test.fixtures.basic.module.types.MyException": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.MyException": ...  # type: ignore


class _fbthrift_compatible_with_MyExceptionWithMessage:
    pass


class MyExceptionWithMessage(_fbthrift_python_exceptions.GeneratedError, _fbthrift_compatible_with_MyExceptionWithMessage, test.fixtures.basic.module.thrift_abstract_types.MyExceptionWithMessage):
    MyIntField: _typing.Final[int] = ...
    MyStringField: _typing.Final[str] = ...
    myStruct: _typing.Final[MyStruct] = ...
    myUnion: _typing.Final[MyUnion] = ...
    def __init__(
        self, *,
        MyIntField: _typing.Optional[int]=...,
        MyStringField: _typing.Optional[str]=...,
        myStruct: _typing.Optional[_fbthrift_compatible_with_MyStruct]=...,
        myUnion: _typing.Optional[_fbthrift_compatible_with_MyUnion]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int, str, MyStruct, MyUnion]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "test.fixtures.basic.module.types.MyExceptionWithMessage": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.MyExceptionWithMessage": ...  # type: ignore


class _fbthrift_compatible_with_ReservedKeyword:
    pass


class ReservedKeyword(_fbthrift_python_types.Struct, _fbthrift_compatible_with_ReservedKeyword, test.fixtures.basic.module.thrift_abstract_types.ReservedKeyword):
    reserved_field: _typing.Final[int] = ...
    def __init__(
        self, *,
        reserved_field: _typing.Optional[int]=...
    ) -> None: ...

    def __call__(
        self, *,
        reserved_field: _typing.Optional[int]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "test.fixtures.basic.module.types.ReservedKeyword": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.ReservedKeyword": ...  # type: ignore


class _fbthrift_compatible_with_UnionToBeRenamed:
    pass


class UnionToBeRenamed(_fbthrift_python_types.Union, _fbthrift_compatible_with_UnionToBeRenamed, test.fixtures.basic.module.thrift_abstract_types.UnionToBeRenamed):
    reserved_field: _typing.Final[int] = ...
    def __init__(
        self, *,
        reserved_field: _typing.Optional[int]=...
    ) -> None: ...


    class Type(enum.Enum):
        EMPTY: UnionToBeRenamed.Type = ...
        reserved_field: UnionToBeRenamed.Type = ...

    @classmethod
    def fromValue(cls, value: _typing.Union[None, int]) -> UnionToBeRenamed: ...
    value: _typing.Final[_typing.Union[None, int]]
    type: _typing.Final[Type]
    def get_type(self) -> Type:...
    def _to_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "test.fixtures.basic.module.types.UnionToBeRenamed": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.UnionToBeRenamed": ...  # type: ignore


FLAG: bool = ...

OFFSET: int = ...

COUNT: int = ...

MASK: int = ...

E: float = ...

DATE: str = ...

AList: _typing.List[int] = ...

ASet: _typing.Set[str] = ...

AMap: _typing.Dict[str, _typing.Sequence[int]] = ...

MyEnumAlias = MyEnum
MyDataItemAlias = MyDataItem


class _fbthrift_FooService_simple_rpc_args(_fbthrift_python_types.Struct):

    def __init__(
        self,
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None]]]: ...


class _fbthrift_FooService_simple_rpc_result(_fbthrift_python_types.Struct):
    success: _typing.Final[None]

    def __init__(
        self, *, success: _typing.Optional[None] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            None,
        ]]]: ...


class _fbthrift_FB303Service_simple_rpc_args(_fbthrift_python_types.Struct):
    int_parameter: _typing.Final[int] = ...

    def __init__(
        self, *,
        int_parameter: _typing.Optional[int]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, int]]]: ...


class _fbthrift_FB303Service_simple_rpc_result(_fbthrift_python_types.Struct):
    success: _typing.Final[ReservedKeyword]

    def __init__(
        self, *, success: _typing.Optional[ReservedKeyword] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            ReservedKeyword,
        ]]]: ...


class _fbthrift_MyService_ping_args(_fbthrift_python_types.Struct):

    def __init__(
        self,
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None]]]: ...


class _fbthrift_MyService_ping_result(_fbthrift_python_types.Struct):
    success: _typing.Final[None]

    def __init__(
        self, *, success: _typing.Optional[None] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            None,
        ]]]: ...


class _fbthrift_MyService_getRandomData_args(_fbthrift_python_types.Struct):

    def __init__(
        self,
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None]]]: ...


class _fbthrift_MyService_getRandomData_result(_fbthrift_python_types.Struct):
    success: _typing.Final[str]

    def __init__(
        self, *, success: _typing.Optional[str] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            str,
        ]]]: ...


class _fbthrift_MyService_sink_args(_fbthrift_python_types.Struct):
    sink: _typing.Final[int] = ...

    def __init__(
        self, *,
        sink: _typing.Optional[int]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, int]]]: ...


class _fbthrift_MyService_sink_result(_fbthrift_python_types.Struct):
    success: _typing.Final[None]

    def __init__(
        self, *, success: _typing.Optional[None] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            None,
        ]]]: ...


class _fbthrift_MyService_putDataById_args(_fbthrift_python_types.Struct):
    id: _typing.Final[int] = ...
    data: _typing.Final[str] = ...

    def __init__(
        self, *,
        id: _typing.Optional[int]=...,
        data: _typing.Optional[str]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, int, str]]]: ...


class _fbthrift_MyService_putDataById_result(_fbthrift_python_types.Struct):
    success: _typing.Final[None]

    def __init__(
        self, *, success: _typing.Optional[None] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            None,
        ]]]: ...


class _fbthrift_MyService_hasDataById_args(_fbthrift_python_types.Struct):
    id: _typing.Final[int] = ...

    def __init__(
        self, *,
        id: _typing.Optional[int]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, int]]]: ...


class _fbthrift_MyService_hasDataById_result(_fbthrift_python_types.Struct):
    success: _typing.Final[bool]

    def __init__(
        self, *, success: _typing.Optional[bool] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            bool,
        ]]]: ...


class _fbthrift_MyService_getDataById_args(_fbthrift_python_types.Struct):
    id: _typing.Final[int] = ...

    def __init__(
        self, *,
        id: _typing.Optional[int]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, int]]]: ...


class _fbthrift_MyService_getDataById_result(_fbthrift_python_types.Struct):
    success: _typing.Final[str]

    def __init__(
        self, *, success: _typing.Optional[str] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            str,
        ]]]: ...


class _fbthrift_MyService_deleteDataById_args(_fbthrift_python_types.Struct):
    id: _typing.Final[int] = ...

    def __init__(
        self, *,
        id: _typing.Optional[int]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, int]]]: ...


class _fbthrift_MyService_deleteDataById_result(_fbthrift_python_types.Struct):
    success: _typing.Final[None]

    def __init__(
        self, *, success: _typing.Optional[None] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            None,
        ]]]: ...


class _fbthrift_MyService_lobDataById_args(_fbthrift_python_types.Struct):
    id: _typing.Final[int] = ...
    data: _typing.Final[str] = ...

    def __init__(
        self, *,
        id: _typing.Optional[int]=...,
        data: _typing.Optional[str]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, int, str]]]: ...


class _fbthrift_MyService_invalid_return_for_hack_args(_fbthrift_python_types.Struct):

    def __init__(
        self,
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None]]]: ...


class _fbthrift_MyService_invalid_return_for_hack_result(_fbthrift_python_types.Struct):
    success: _typing.Final[_typing.AbstractSet[float]]

    def __init__(
        self, *, success: _typing.Optional[_typing.AbstractSet[float]] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            _typing.AbstractSet[float],
        ]]]: ...


class _fbthrift_MyService_rpc_skipped_codegen_args(_fbthrift_python_types.Struct):

    def __init__(
        self,
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None]]]: ...


class _fbthrift_MyService_rpc_skipped_codegen_result(_fbthrift_python_types.Struct):
    success: _typing.Final[None]

    def __init__(
        self, *, success: _typing.Optional[None] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            None,
        ]]]: ...


class _fbthrift_DbMixedStackArguments_getDataByKey0_args(_fbthrift_python_types.Struct):
    key: _typing.Final[str] = ...

    def __init__(
        self, *,
        key: _typing.Optional[str]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, str]]]: ...


class _fbthrift_DbMixedStackArguments_getDataByKey0_result(_fbthrift_python_types.Struct):
    success: _typing.Final[bytes]

    def __init__(
        self, *, success: _typing.Optional[bytes] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            bytes,
        ]]]: ...


class _fbthrift_DbMixedStackArguments_getDataByKey1_args(_fbthrift_python_types.Struct):
    key: _typing.Final[str] = ...

    def __init__(
        self, *,
        key: _typing.Optional[str]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, str]]]: ...


class _fbthrift_DbMixedStackArguments_getDataByKey1_result(_fbthrift_python_types.Struct):
    success: _typing.Final[bytes]

    def __init__(
        self, *, success: _typing.Optional[bytes] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            bytes,
        ]]]: ...
