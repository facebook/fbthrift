#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from __future__ import annotations

import typing as _typing

import a.thrift_types as _fbthrift_current_module
import folly.iobuf as _fbthrift_iobuf
import thrift.python.types as _fbthrift_python_types
import thrift.python.exceptions as _fbthrift_python_exceptions

import b.thrift_types as _fbthrift__b__thrift_types

import c.thrift_types as _fbthrift__c__thrift_types
import typeshed_three
import typeshed_two
import typeshed_one

from a.thrift_enums import *


class _fbthrift_compatible_with_MyStruct:
    pass


class MyStruct(_fbthrift_python_types.Struct, _fbthrift_compatible_with_MyStruct):
    a: _typing.Final[typeshed_one.AdapterOneType[_fbthrift__b__thrift_types.B]] = ...
    b: _typing.Final[typeshed_three.AdapterThreeType[_fbthrift__c__thrift_types.C1]] = ...
    c: _typing.Final[typeshed_two.AdapterTwoType[_fbthrift__c__thrift_types.C2]] = ...
    def __init__(
        self, *,
        a: _typing.Optional[typeshed_one.AdapterOneType[_fbthrift__b__thrift_types.B]]=...,
        b: _typing.Optional[typeshed_three.AdapterThreeType[_fbthrift__c__thrift_types.C1]]=...,
        c: _typing.Optional[typeshed_two.AdapterTwoType[_fbthrift__c__thrift_types.C2]]=...
    ) -> None: ...

    def __call__(
        self, *,
        a: _typing.Optional[typeshed_one.AdapterOneType[_fbthrift__b__thrift_types.B]]=...,
        b: _typing.Optional[typeshed_three.AdapterThreeType[_fbthrift__c__thrift_types.C1]]=...,
        c: _typing.Optional[typeshed_two.AdapterTwoType[_fbthrift__c__thrift_types.C2]]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[typeshed_one.AdapterOneType[_fbthrift__b__thrift_types.B], typeshed_three.AdapterThreeType[_fbthrift__c__thrift_types.C1], typeshed_two.AdapterTwoType[_fbthrift__c__thrift_types.C2]]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "a.types.MyStruct": ...  # type: ignore
    def _to_py_deprecated(self) -> "a.ttypes.MyStruct": ...  # type: ignore


class _fbthrift_MyService_adapted_return_args(_fbthrift_python_types.Struct):

    def __init__(
        self,
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None]]]: ...


class _fbthrift_MyService_adapted_return_result(_fbthrift_python_types.Struct):
    success: _typing.Final[typeshed_one.AdapterOneType[_fbthrift__b__thrift_types.B]]

    def __init__(
        self, *, success: _typing.Optional[typeshed_one.AdapterOneType[_fbthrift__b__thrift_types.B]] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            typeshed_one.AdapterOneType[_fbthrift__b__thrift_types.B],
        ]]]: ...


class _fbthrift_MyService_adapted_param_args(_fbthrift_python_types.Struct):
    param: _typing.Final[typeshed_two.AdapterTwoType[_fbthrift__c__thrift_types.C2]] = ...

    def __init__(
        self, *,
        param: _typing.Optional[typeshed_two.AdapterTwoType[_fbthrift__c__thrift_types.C2]]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, typeshed_two.AdapterTwoType[_fbthrift__c__thrift_types.C2]]]]: ...


class _fbthrift_MyService_adapted_param_result(_fbthrift_python_types.Struct):
    success: _typing.Final[None]

    def __init__(
        self, *, success: _typing.Optional[None] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            None,
        ]]]: ...
