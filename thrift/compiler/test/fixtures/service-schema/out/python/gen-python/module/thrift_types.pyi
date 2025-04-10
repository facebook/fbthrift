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
import thrift.python.exceptions as _fbthrift_python_exceptions

import include.thrift_types as _fbthrift__include__thrift_types

from module.thrift_enums import (
    Result as _fbthrift_Result,
)
Result = _fbthrift_Result


class CustomException(_fbthrift_python_exceptions.GeneratedError, _fbthrift_python_abstract_types.CustomException):
    name: _typing.Final[str] = ...
    result: _typing.Final[_fbthrift_Result] = ...
    def __init__(
        self, *,
        name: _typing.Optional[str]=...,
        result: _typing.Optional[Result]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[str, _fbthrift_Result]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "module.thrift_mutable_types.CustomException": ...  # type: ignore
    def _to_py3(self) -> "module.types.CustomException": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.CustomException": ...  # type: ignore
_fbthrift_CustomException = CustomException


_fbthrift_schema_b747839c13cb3aa5: bytes = ...


class _fbthrift_PrimitivesService_init_args(_fbthrift_python_types.Struct):
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


class _fbthrift_PrimitivesService_init_result(_fbthrift_python_types.Struct):
    success: _typing.Final[int]

    def __init__(
        self, *, success: _typing.Optional[int] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            int,
        ]]]: ...


class _fbthrift_PrimitivesService_method_that_throws_args(_fbthrift_python_types.Struct):

    def __init__(
        self,
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None]]]: ...


class _fbthrift_PrimitivesService_method_that_throws_result(_fbthrift_python_types.Struct):
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


class _fbthrift_PrimitivesService_return_void_method_args(_fbthrift_python_types.Struct):
    id: _typing.Final[int] = ...

    def __init__(
        self, *,
        id: _typing.Optional[int]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, int]]]: ...


class _fbthrift_PrimitivesService_return_void_method_result(_fbthrift_python_types.Struct):
    success: _typing.Final[None]

    def __init__(
        self, *, success: _typing.Optional[None] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            None,
        ]]]: ...
