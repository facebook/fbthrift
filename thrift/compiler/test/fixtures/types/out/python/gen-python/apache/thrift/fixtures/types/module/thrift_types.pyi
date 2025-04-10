#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from __future__ import annotations

import typing as _typing

import folly.iobuf as _fbthrift_iobuf
import apache.thrift.fixtures.types.module.thrift_abstract_types as _fbthrift_python_abstract_types
import thrift.python.types as _fbthrift_python_types
import thrift.python.exceptions as _fbthrift_python_exceptions

import apache.thrift.fixtures.types.included.thrift_types as _fbthrift__apache__thrift__fixtures__types__included__thrift_types

from apache.thrift.fixtures.types.module.thrift_enums import (
    has_bitwise_ops as _fbthrift_has_bitwise_ops,
    is_unscoped as _fbthrift_is_unscoped,
    MyForwardRefEnum as _fbthrift_MyForwardRefEnum,
)
has_bitwise_ops = _fbthrift_has_bitwise_ops
is_unscoped = _fbthrift_is_unscoped
MyForwardRefEnum = _fbthrift_MyForwardRefEnum


class empty_struct(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.empty_struct):
    def __init__(
        self,
    ) -> None: ...

    def __call__(
        self,
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[None]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.empty_struct": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.empty_struct": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.empty_struct": ...  # type: ignore
_fbthrift_empty_struct = empty_struct

class decorated_struct(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.decorated_struct):
    field: _typing.Final[str] = ...
    def __init__(
        self, *,
        field: _typing.Optional[str]=...
    ) -> None: ...

    def __call__(
        self, *,
        field: _typing.Optional[str]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[str]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.decorated_struct": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.decorated_struct": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.decorated_struct": ...  # type: ignore
_fbthrift_decorated_struct = decorated_struct

class ContainerStruct(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.ContainerStruct):
    fieldB: _typing.Final[_typing.Sequence[int]] = ...
    fieldC: _typing.Final[_typing.Sequence[int]] = ...
    fieldD: _typing.Final[_typing.Sequence[int]] = ...
    fieldE: _typing.Final[_typing.Sequence[int]] = ...
    fieldF: _typing.Final[_typing.AbstractSet[int]] = ...
    fieldG: _typing.Final[_typing.Mapping[int, str]] = ...
    fieldH: _typing.Final[_typing.Mapping[int, str]] = ...
    fieldA: _typing.Final[_typing.Sequence[int]] = ...
    def __init__(
        self, *,
        fieldB: _typing.Optional[_typing.Sequence[int]]=...,
        fieldC: _typing.Optional[_typing.Sequence[int]]=...,
        fieldD: _typing.Optional[_typing.Sequence[int]]=...,
        fieldE: _typing.Optional[_typing.Sequence[int]]=...,
        fieldF: _typing.Optional[_typing.AbstractSet[int]]=...,
        fieldG: _typing.Optional[_typing.Mapping[int, str]]=...,
        fieldH: _typing.Optional[_typing.Mapping[int, str]]=...,
        fieldA: _typing.Optional[_typing.Sequence[int]]=...
    ) -> None: ...

    def __call__(
        self, *,
        fieldB: _typing.Optional[_typing.Sequence[int]]=...,
        fieldC: _typing.Optional[_typing.Sequence[int]]=...,
        fieldD: _typing.Optional[_typing.Sequence[int]]=...,
        fieldE: _typing.Optional[_typing.Sequence[int]]=...,
        fieldF: _typing.Optional[_typing.AbstractSet[int]]=...,
        fieldG: _typing.Optional[_typing.Mapping[int, str]]=...,
        fieldH: _typing.Optional[_typing.Mapping[int, str]]=...,
        fieldA: _typing.Optional[_typing.Sequence[int]]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[_typing.Sequence[int], _typing.Sequence[int], _typing.Sequence[int], _typing.Sequence[int], _typing.AbstractSet[int], _typing.Mapping[int, str], _typing.Mapping[int, str], _typing.Sequence[int]]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.ContainerStruct": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.ContainerStruct": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.ContainerStruct": ...  # type: ignore
_fbthrift_ContainerStruct = ContainerStruct

class CppTypeStruct(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.CppTypeStruct):
    fieldA: _typing.Final[_typing.Sequence[int]] = ...
    def __init__(
        self, *,
        fieldA: _typing.Optional[_typing.Sequence[int]]=...
    ) -> None: ...

    def __call__(
        self, *,
        fieldA: _typing.Optional[_typing.Sequence[int]]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[_typing.Sequence[int]]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.CppTypeStruct": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.CppTypeStruct": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.CppTypeStruct": ...  # type: ignore
_fbthrift_CppTypeStruct = CppTypeStruct

class VirtualStruct(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.VirtualStruct):
    MyIntField: _typing.Final[int] = ...
    def __init__(
        self, *,
        MyIntField: _typing.Optional[int]=...
    ) -> None: ...

    def __call__(
        self, *,
        MyIntField: _typing.Optional[int]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.VirtualStruct": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.VirtualStruct": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.VirtualStruct": ...  # type: ignore
_fbthrift_VirtualStruct = VirtualStruct

class MyStructWithForwardRefEnum(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.MyStructWithForwardRefEnum):
    a: _typing.Final[_fbthrift_MyForwardRefEnum] = ...
    b: _typing.Final[_fbthrift_MyForwardRefEnum] = ...
    def __init__(
        self, *,
        a: _typing.Optional[MyForwardRefEnum]=...,
        b: _typing.Optional[MyForwardRefEnum]=...
    ) -> None: ...

    def __call__(
        self, *,
        a: _typing.Optional[MyForwardRefEnum]=...,
        b: _typing.Optional[MyForwardRefEnum]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[_fbthrift_MyForwardRefEnum, _fbthrift_MyForwardRefEnum]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.MyStructWithForwardRefEnum": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.MyStructWithForwardRefEnum": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.MyStructWithForwardRefEnum": ...  # type: ignore
_fbthrift_MyStructWithForwardRefEnum = MyStructWithForwardRefEnum

class TrivialNumeric(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.TrivialNumeric):
    a: _typing.Final[int] = ...
    b: _typing.Final[bool] = ...
    def __init__(
        self, *,
        a: _typing.Optional[int]=...,
        b: _typing.Optional[bool]=...
    ) -> None: ...

    def __call__(
        self, *,
        a: _typing.Optional[int]=...,
        b: _typing.Optional[bool]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int, bool]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.TrivialNumeric": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.TrivialNumeric": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.TrivialNumeric": ...  # type: ignore
_fbthrift_TrivialNumeric = TrivialNumeric

class TrivialNestedWithDefault(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.TrivialNestedWithDefault):
    z: _typing.Final[int] = ...
    n: _typing.Final[_fbthrift_TrivialNumeric] = ...
    def __init__(
        self, *,
        z: _typing.Optional[int]=...,
        n: _typing.Optional[TrivialNumeric]=...
    ) -> None: ...

    def __call__(
        self, *,
        z: _typing.Optional[int]=...,
        n: _typing.Optional[TrivialNumeric]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int, _fbthrift_TrivialNumeric]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.TrivialNestedWithDefault": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.TrivialNestedWithDefault": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.TrivialNestedWithDefault": ...  # type: ignore
_fbthrift_TrivialNestedWithDefault = TrivialNestedWithDefault

class ComplexString(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.ComplexString):
    a: _typing.Final[str] = ...
    b: _typing.Final[_typing.Mapping[str, int]] = ...
    def __init__(
        self, *,
        a: _typing.Optional[str]=...,
        b: _typing.Optional[_typing.Mapping[str, int]]=...
    ) -> None: ...

    def __call__(
        self, *,
        a: _typing.Optional[str]=...,
        b: _typing.Optional[_typing.Mapping[str, int]]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[str, _typing.Mapping[str, int]]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.ComplexString": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.ComplexString": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.ComplexString": ...  # type: ignore
_fbthrift_ComplexString = ComplexString

class ComplexNestedWithDefault(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.ComplexNestedWithDefault):
    z: _typing.Final[str] = ...
    n: _typing.Final[_fbthrift_ComplexString] = ...
    def __init__(
        self, *,
        z: _typing.Optional[str]=...,
        n: _typing.Optional[ComplexString]=...
    ) -> None: ...

    def __call__(
        self, *,
        z: _typing.Optional[str]=...,
        n: _typing.Optional[ComplexString]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[str, _fbthrift_ComplexString]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.ComplexNestedWithDefault": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.ComplexNestedWithDefault": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.ComplexNestedWithDefault": ...  # type: ignore
_fbthrift_ComplexNestedWithDefault = ComplexNestedWithDefault

class MinPadding(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.MinPadding):
    small: _typing.Final[int] = ...
    big: _typing.Final[int] = ...
    medium: _typing.Final[int] = ...
    biggish: _typing.Final[int] = ...
    tiny: _typing.Final[int] = ...
    def __init__(
        self, *,
        small: _typing.Optional[int]=...,
        big: _typing.Optional[int]=...,
        medium: _typing.Optional[int]=...,
        biggish: _typing.Optional[int]=...,
        tiny: _typing.Optional[int]=...
    ) -> None: ...

    def __call__(
        self, *,
        small: _typing.Optional[int]=...,
        big: _typing.Optional[int]=...,
        medium: _typing.Optional[int]=...,
        biggish: _typing.Optional[int]=...,
        tiny: _typing.Optional[int]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int, int, int, int, int]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.MinPadding": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.MinPadding": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.MinPadding": ...  # type: ignore
_fbthrift_MinPadding = MinPadding

class MinPaddingWithCustomType(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.MinPaddingWithCustomType):
    small: _typing.Final[int] = ...
    biggish: _typing.Final[int] = ...
    medium: _typing.Final[int] = ...
    big: _typing.Final[int] = ...
    tiny: _typing.Final[int] = ...
    def __init__(
        self, *,
        small: _typing.Optional[int]=...,
        biggish: _typing.Optional[int]=...,
        medium: _typing.Optional[int]=...,
        big: _typing.Optional[int]=...,
        tiny: _typing.Optional[int]=...
    ) -> None: ...

    def __call__(
        self, *,
        small: _typing.Optional[int]=...,
        biggish: _typing.Optional[int]=...,
        medium: _typing.Optional[int]=...,
        big: _typing.Optional[int]=...,
        tiny: _typing.Optional[int]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int, int, int, int, int]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.MinPaddingWithCustomType": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.MinPaddingWithCustomType": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.MinPaddingWithCustomType": ...  # type: ignore
_fbthrift_MinPaddingWithCustomType = MinPaddingWithCustomType

class MyStruct(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.MyStruct):
    MyIntField: _typing.Final[int] = ...
    MyStringField: _typing.Final[str] = ...
    majorVer: _typing.Final[int] = ...
    data: _typing.Final[_fbthrift_MyDataItem] = ...
    def __init__(
        self, *,
        MyIntField: _typing.Optional[int]=...,
        MyStringField: _typing.Optional[str]=...,
        majorVer: _typing.Optional[int]=...,
        data: _typing.Optional[MyDataItem]=...
    ) -> None: ...

    def __call__(
        self, *,
        MyIntField: _typing.Optional[int]=...,
        MyStringField: _typing.Optional[str]=...,
        majorVer: _typing.Optional[int]=...,
        data: _typing.Optional[MyDataItem]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int, str, int, _fbthrift_MyDataItem]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.MyStruct": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.MyStruct": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.MyStruct": ...  # type: ignore
_fbthrift_MyStruct = MyStruct

class MyDataItem(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.MyDataItem):
    def __init__(
        self,
    ) -> None: ...

    def __call__(
        self,
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[None]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.MyDataItem": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.MyDataItem": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.MyDataItem": ...  # type: ignore
_fbthrift_MyDataItem = MyDataItem

class Renaming(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.Renaming):
    foo: _typing.Final[int] = ...
    def __init__(
        self, *,
        foo: _typing.Optional[int]=...
    ) -> None: ...

    def __call__(
        self, *,
        foo: _typing.Optional[int]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.Renaming": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.Renaming": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.Renaming": ...  # type: ignore
_fbthrift_Renaming = Renaming

class AnnotatedTypes(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.AnnotatedTypes):
    binary_field: _typing.Final[bytes] = ...
    list_field: _typing.Final[_typing.Sequence[_typing.Mapping[int, str]]] = ...
    def __init__(
        self, *,
        binary_field: _typing.Optional[bytes]=...,
        list_field: _typing.Optional[_typing.Sequence[_typing.Mapping[int, str]]]=...
    ) -> None: ...

    def __call__(
        self, *,
        binary_field: _typing.Optional[bytes]=...,
        list_field: _typing.Optional[_typing.Sequence[_typing.Mapping[int, str]]]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[bytes, _typing.Sequence[_typing.Mapping[int, str]]]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.AnnotatedTypes": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.AnnotatedTypes": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.AnnotatedTypes": ...  # type: ignore
_fbthrift_AnnotatedTypes = AnnotatedTypes

class ForwardUsageRoot(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.ForwardUsageRoot):
    ForwardUsageStruct: _typing.Final[_typing.Optional[_fbthrift_ForwardUsageStruct]] = ...
    ForwardUsageByRef: _typing.Final[_typing.Optional[_fbthrift_ForwardUsageByRef]] = ...
    def __init__(
        self, *,
        ForwardUsageStruct: _typing.Optional[ForwardUsageStruct]=...,
        ForwardUsageByRef: _typing.Optional[ForwardUsageByRef]=...
    ) -> None: ...

    def __call__(
        self, *,
        ForwardUsageStruct: _typing.Optional[ForwardUsageStruct]=...,
        ForwardUsageByRef: _typing.Optional[ForwardUsageByRef]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[_fbthrift_ForwardUsageStruct, _fbthrift_ForwardUsageByRef]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.ForwardUsageRoot": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.ForwardUsageRoot": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.ForwardUsageRoot": ...  # type: ignore
_fbthrift_ForwardUsageRoot = ForwardUsageRoot

class ForwardUsageStruct(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.ForwardUsageStruct):
    foo: _typing.Final[_typing.Optional[_fbthrift_ForwardUsageRoot]] = ...
    def __init__(
        self, *,
        foo: _typing.Optional[ForwardUsageRoot]=...
    ) -> None: ...

    def __call__(
        self, *,
        foo: _typing.Optional[ForwardUsageRoot]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[_fbthrift_ForwardUsageRoot]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.ForwardUsageStruct": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.ForwardUsageStruct": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.ForwardUsageStruct": ...  # type: ignore
_fbthrift_ForwardUsageStruct = ForwardUsageStruct

class ForwardUsageByRef(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.ForwardUsageByRef):
    foo: _typing.Final[_typing.Optional[_fbthrift_ForwardUsageRoot]] = ...
    def __init__(
        self, *,
        foo: _typing.Optional[ForwardUsageRoot]=...
    ) -> None: ...

    def __call__(
        self, *,
        foo: _typing.Optional[ForwardUsageRoot]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[_fbthrift_ForwardUsageRoot]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.ForwardUsageByRef": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.ForwardUsageByRef": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.ForwardUsageByRef": ...  # type: ignore
_fbthrift_ForwardUsageByRef = ForwardUsageByRef

class IncompleteMap(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.IncompleteMap):
    field: _typing.Final[_typing.Optional[_typing.Mapping[int, _fbthrift_IncompleteMapDep]]] = ...
    def __init__(
        self, *,
        field: _typing.Optional[_typing.Mapping[int, IncompleteMapDep]]=...
    ) -> None: ...

    def __call__(
        self, *,
        field: _typing.Optional[_typing.Mapping[int, IncompleteMapDep]]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[_typing.Mapping[int, _fbthrift_IncompleteMapDep]]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.IncompleteMap": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.IncompleteMap": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.IncompleteMap": ...  # type: ignore
_fbthrift_IncompleteMap = IncompleteMap

class IncompleteMapDep(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.IncompleteMapDep):
    def __init__(
        self,
    ) -> None: ...

    def __call__(
        self,
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[None]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.IncompleteMapDep": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.IncompleteMapDep": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.IncompleteMapDep": ...  # type: ignore
_fbthrift_IncompleteMapDep = IncompleteMapDep

class CompleteMap(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.CompleteMap):
    field: _typing.Final[_typing.Optional[_typing.Mapping[int, _fbthrift_CompleteMapDep]]] = ...
    def __init__(
        self, *,
        field: _typing.Optional[_typing.Mapping[int, CompleteMapDep]]=...
    ) -> None: ...

    def __call__(
        self, *,
        field: _typing.Optional[_typing.Mapping[int, CompleteMapDep]]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[_typing.Mapping[int, _fbthrift_CompleteMapDep]]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.CompleteMap": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.CompleteMap": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.CompleteMap": ...  # type: ignore
_fbthrift_CompleteMap = CompleteMap

class CompleteMapDep(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.CompleteMapDep):
    def __init__(
        self,
    ) -> None: ...

    def __call__(
        self,
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[None]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.CompleteMapDep": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.CompleteMapDep": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.CompleteMapDep": ...  # type: ignore
_fbthrift_CompleteMapDep = CompleteMapDep

class IncompleteList(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.IncompleteList):
    field: _typing.Final[_typing.Optional[_typing.Sequence[_fbthrift_IncompleteListDep]]] = ...
    def __init__(
        self, *,
        field: _typing.Optional[_typing.Sequence[IncompleteListDep]]=...
    ) -> None: ...

    def __call__(
        self, *,
        field: _typing.Optional[_typing.Sequence[IncompleteListDep]]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[_typing.Sequence[_fbthrift_IncompleteListDep]]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.IncompleteList": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.IncompleteList": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.IncompleteList": ...  # type: ignore
_fbthrift_IncompleteList = IncompleteList

class IncompleteListDep(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.IncompleteListDep):
    def __init__(
        self,
    ) -> None: ...

    def __call__(
        self,
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[None]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.IncompleteListDep": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.IncompleteListDep": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.IncompleteListDep": ...  # type: ignore
_fbthrift_IncompleteListDep = IncompleteListDep

class CompleteList(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.CompleteList):
    field: _typing.Final[_typing.Optional[_typing.Sequence[_fbthrift_CompleteListDep]]] = ...
    def __init__(
        self, *,
        field: _typing.Optional[_typing.Sequence[CompleteListDep]]=...
    ) -> None: ...

    def __call__(
        self, *,
        field: _typing.Optional[_typing.Sequence[CompleteListDep]]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[_typing.Sequence[_fbthrift_CompleteListDep]]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.CompleteList": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.CompleteList": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.CompleteList": ...  # type: ignore
_fbthrift_CompleteList = CompleteList

class CompleteListDep(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.CompleteListDep):
    def __init__(
        self,
    ) -> None: ...

    def __call__(
        self,
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[None]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.CompleteListDep": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.CompleteListDep": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.CompleteListDep": ...  # type: ignore
_fbthrift_CompleteListDep = CompleteListDep

class AdaptedList(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.AdaptedList):
    field: _typing.Final[_typing.Optional[_typing.Sequence[_fbthrift_AdaptedListDep]]] = ...
    def __init__(
        self, *,
        field: _typing.Optional[_typing.Sequence[AdaptedListDep]]=...
    ) -> None: ...

    def __call__(
        self, *,
        field: _typing.Optional[_typing.Sequence[AdaptedListDep]]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[_typing.Sequence[_fbthrift_AdaptedListDep]]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.AdaptedList": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.AdaptedList": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.AdaptedList": ...  # type: ignore
_fbthrift_AdaptedList = AdaptedList

class AdaptedListDep(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.AdaptedListDep):
    field: _typing.Final[_fbthrift_AdaptedList] = ...
    def __init__(
        self, *,
        field: _typing.Optional[AdaptedList]=...
    ) -> None: ...

    def __call__(
        self, *,
        field: _typing.Optional[AdaptedList]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[_fbthrift_AdaptedList]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.AdaptedListDep": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.AdaptedListDep": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.AdaptedListDep": ...  # type: ignore
_fbthrift_AdaptedListDep = AdaptedListDep

class DependentAdaptedList(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.DependentAdaptedList):
    field: _typing.Final[_typing.Optional[_typing.Sequence[_fbthrift_DependentAdaptedListDep]]] = ...
    def __init__(
        self, *,
        field: _typing.Optional[_typing.Sequence[DependentAdaptedListDep]]=...
    ) -> None: ...

    def __call__(
        self, *,
        field: _typing.Optional[_typing.Sequence[DependentAdaptedListDep]]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[_typing.Sequence[_fbthrift_DependentAdaptedListDep]]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.DependentAdaptedList": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.DependentAdaptedList": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.DependentAdaptedList": ...  # type: ignore
_fbthrift_DependentAdaptedList = DependentAdaptedList

class DependentAdaptedListDep(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.DependentAdaptedListDep):
    field: _typing.Final[_typing.Optional[int]] = ...
    def __init__(
        self, *,
        field: _typing.Optional[int]=...
    ) -> None: ...

    def __call__(
        self, *,
        field: _typing.Optional[int]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.DependentAdaptedListDep": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.DependentAdaptedListDep": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.DependentAdaptedListDep": ...  # type: ignore
_fbthrift_DependentAdaptedListDep = DependentAdaptedListDep

class AllocatorAware(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.AllocatorAware):
    aa_list: _typing.Final[_typing.Sequence[int]] = ...
    aa_set: _typing.Final[_typing.AbstractSet[int]] = ...
    aa_map: _typing.Final[_typing.Mapping[int, int]] = ...
    aa_string: _typing.Final[str] = ...
    not_a_container: _typing.Final[int] = ...
    aa_unique: _typing.Final[int] = ...
    aa_shared: _typing.Final[int] = ...
    def __init__(
        self, *,
        aa_list: _typing.Optional[_typing.Sequence[int]]=...,
        aa_set: _typing.Optional[_typing.AbstractSet[int]]=...,
        aa_map: _typing.Optional[_typing.Mapping[int, int]]=...,
        aa_string: _typing.Optional[str]=...,
        not_a_container: _typing.Optional[int]=...,
        aa_unique: _typing.Optional[int]=...,
        aa_shared: _typing.Optional[int]=...
    ) -> None: ...

    def __call__(
        self, *,
        aa_list: _typing.Optional[_typing.Sequence[int]]=...,
        aa_set: _typing.Optional[_typing.AbstractSet[int]]=...,
        aa_map: _typing.Optional[_typing.Mapping[int, int]]=...,
        aa_string: _typing.Optional[str]=...,
        not_a_container: _typing.Optional[int]=...,
        aa_unique: _typing.Optional[int]=...,
        aa_shared: _typing.Optional[int]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[_typing.Sequence[int], _typing.AbstractSet[int], _typing.Mapping[int, int], str, int, int, int]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.AllocatorAware": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.AllocatorAware": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.AllocatorAware": ...  # type: ignore
_fbthrift_AllocatorAware = AllocatorAware

class AllocatorAware2(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.AllocatorAware2):
    not_a_container: _typing.Final[int] = ...
    box_field: _typing.Final[_typing.Optional[int]] = ...
    def __init__(
        self, *,
        not_a_container: _typing.Optional[int]=...,
        box_field: _typing.Optional[int]=...
    ) -> None: ...

    def __call__(
        self, *,
        not_a_container: _typing.Optional[int]=...,
        box_field: _typing.Optional[int]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int, int]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.AllocatorAware2": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.AllocatorAware2": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.AllocatorAware2": ...  # type: ignore
_fbthrift_AllocatorAware2 = AllocatorAware2

class TypedefStruct(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.TypedefStruct):
    i32_field: _typing.Final[int] = ...
    IntTypedef_field: _typing.Final[int] = ...
    UintTypedef_field: _typing.Final[int] = ...
    def __init__(
        self, *,
        i32_field: _typing.Optional[int]=...,
        IntTypedef_field: _typing.Optional[int]=...,
        UintTypedef_field: _typing.Optional[int]=...
    ) -> None: ...

    def __call__(
        self, *,
        i32_field: _typing.Optional[int]=...,
        IntTypedef_field: _typing.Optional[int]=...,
        UintTypedef_field: _typing.Optional[int]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int, int, int]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.TypedefStruct": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.TypedefStruct": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.TypedefStruct": ...  # type: ignore
_fbthrift_TypedefStruct = TypedefStruct

class StructWithDoubleUnderscores(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.StructWithDoubleUnderscores):
    _StructWithDoubleUnderscores__field: _typing.Final[int] = ...
    def __init__(
        self, *,
        _StructWithDoubleUnderscores__field: _typing.Optional[int]=...
    ) -> None: ...

    def __call__(
        self, *,
        _StructWithDoubleUnderscores__field: _typing.Optional[int]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "apache.thrift.fixtures.types.module.thrift_mutable_types.StructWithDoubleUnderscores": ...  # type: ignore
    def _to_py3(self) -> "apache.thrift.fixtures.types.module.types.StructWithDoubleUnderscores": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.StructWithDoubleUnderscores": ...  # type: ignore
_fbthrift_StructWithDoubleUnderscores = StructWithDoubleUnderscores

TBinary = bytes
IntTypedef = int
UintTypedef = int
SomeListOfTypeMap_2468 = _typing.List[_typing.Mapping[int, str]]
TBinary_8623 = bytes
i32_9314 = int
list_i32_9187 = _typing.List[int]
map_i32_i32_9565 = _typing.Dict[int, int]
map_i32_string_1261 = _typing.Dict[int, str]
set_i32_7070 = _typing.Set[int]
set_i32_7194 = _typing.Set[int]
string_5252 = str


class _fbthrift_SomeService_bounce_map_args(_fbthrift_python_types.Struct):
    m: _typing.Final[_typing.Mapping[int, str]] = ...

    def __init__(
        self, *,
        m: _typing.Optional[_typing.Mapping[int, str]]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, _typing.Mapping[int, str]]]]: ...


class _fbthrift_SomeService_bounce_map_result(_fbthrift_python_types.Struct):
    success: _typing.Final[_typing.Mapping[int, str]]

    def __init__(
        self, *, success: _typing.Optional[_typing.Mapping[int, str]] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            _typing.Mapping[int, str],
        ]]]: ...


class _fbthrift_SomeService_binary_keyed_map_args(_fbthrift_python_types.Struct):
    r: _typing.Final[_typing.Sequence[int]] = ...

    def __init__(
        self, *,
        r: _typing.Optional[_typing.Sequence[int]]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, _typing.Sequence[int]]]]: ...


class _fbthrift_SomeService_binary_keyed_map_result(_fbthrift_python_types.Struct):
    success: _typing.Final[_typing.Mapping[bytes, int]]

    def __init__(
        self, *, success: _typing.Optional[_typing.Mapping[bytes, int]] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            _typing.Mapping[bytes, int],
        ]]]: ...
