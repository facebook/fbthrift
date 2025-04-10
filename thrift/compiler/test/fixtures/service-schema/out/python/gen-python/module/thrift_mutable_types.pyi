#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from __future__ import annotations

import typing as _typing

import folly.iobuf as _fbthrift_iobuf
import module.thrift_abstract_types as _fbthrift_python_abstract_types
import thrift.python.types as _fbthrift_python_types
import thrift.python.mutable_types as _fbthrift_python_mutable_types
import thrift.python.mutable_exceptions as _fbthrift_python_mutable_exceptions
import thrift.python.mutable_containers as _fbthrift_python_mutable_containers

import include.thrift_mutable_types as _fbthrift__include__thrift_mutable_types

from module.thrift_enums import (
    Result as _fbthrift_Result,
)
Result = _fbthrift_Result


class CustomException(_fbthrift_python_mutable_exceptions.MutableGeneratedError, _fbthrift_python_abstract_types.CustomException):
    @property
    def name(self) -> str: ...
    @name.setter
    def name(self, value: str) -> None: ...

    @property
    def result(self) -> _fbthrift_Result: ...
    @result.setter
    def result(self, value: _fbthrift_Result) -> None: ...

    def __init__(
        self, *,
        name: _typing.Optional[str]=...,
        result: _typing.Optional[Result]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[str, _fbthrift_Result]]]: ...
    def _to_python(self) -> "module.thrift_types.CustomException": ...  # type: ignore
    def _to_mutable_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "module.types.CustomException": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.CustomException": ...  # type: ignore
_fbthrift_CustomException = CustomException


_fbthrift_schema_b747839c13cb3aa5: bytes = ...


class _fbthrift_PrimitivesService_init_args(_fbthrift_python_mutable_types.MutableStruct):
    param0: _typing.Final[int] = ...
    param1: _typing.Final[int] = ...

    def __init__(
        self, *,
        param0: _typing.Optional[int]=...,
        param1: _typing.Optional[int]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, int, int]]]: ...


class _fbthrift_PrimitivesService_init_result(_fbthrift_python_mutable_types.MutableStruct):
    success: _typing.Final[int]

    def __init__(
        self, *, success: _typing.Optional[int] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            int,
        ]]]: ...


class _fbthrift_PrimitivesService_method_that_throws_args(_fbthrift_python_mutable_types.MutableStruct):

    def __init__(
        self,
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None]]]: ...


class _fbthrift_PrimitivesService_method_that_throws_result(_fbthrift_python_mutable_types.MutableStruct):
    success: _typing.Final[_fbthrift_Result]
    e: _typing.Final[_fbthrift_CustomException]

    def __init__(
        self, *, success: _typing.Optional[_fbthrift_Result] = ..., e: _typing.Optional[_fbthrift_CustomException]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            _fbthrift_Result,
            _fbthrift_CustomException,
        ]]]: ...


class _fbthrift_PrimitivesService_return_void_method_args(_fbthrift_python_mutable_types.MutableStruct):
    id: _typing.Final[int] = ...

    def __init__(
        self, *,
        id: _typing.Optional[int]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, int]]]: ...


class _fbthrift_PrimitivesService_return_void_method_result(_fbthrift_python_mutable_types.MutableStruct):
    success: _typing.Final[None]

    def __init__(
        self, *, success: _typing.Optional[None] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            None,
        ]]]: ...
