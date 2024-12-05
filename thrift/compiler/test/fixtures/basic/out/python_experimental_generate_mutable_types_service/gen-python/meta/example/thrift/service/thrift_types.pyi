#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from __future__ import annotations

import typing as _typing

import meta.example.thrift.service.thrift_types as _fbthrift_current_module
import folly.iobuf as _fbthrift_iobuf
import thrift.python.types as _fbthrift_python_types
import thrift.python.exceptions as _fbthrift_python_exceptions

from meta.example.thrift.service.thrift_enums import *


class _fbthrift_compatible_with_EchoRequest:
    pass


class EchoRequest(_fbthrift_python_types.Struct, _fbthrift_compatible_with_EchoRequest):
    text: _typing.Final[str] = ...
    def __init__(
        self, *,
        text: _typing.Optional[str]=...
    ) -> None: ...

    def __call__(
        self, *,
        text: _typing.Optional[str]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[str]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "meta.example.thrift.service.thrift_mutable_types.EchoRequest": ...  # type: ignore
    def _to_py3(self) -> "meta.example.thrift.service.types.EchoRequest": ...  # type: ignore
    def _to_py_deprecated(self) -> "service.ttypes.EchoRequest": ...  # type: ignore


class _fbthrift_compatible_with_EchoResponse:
    pass


class EchoResponse(_fbthrift_python_types.Struct, _fbthrift_compatible_with_EchoResponse):
    text: _typing.Final[str] = ...
    def __init__(
        self, *,
        text: _typing.Optional[str]=...
    ) -> None: ...

    def __call__(
        self, *,
        text: _typing.Optional[str]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[str]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "meta.example.thrift.service.thrift_mutable_types.EchoResponse": ...  # type: ignore
    def _to_py3(self) -> "meta.example.thrift.service.types.EchoResponse": ...  # type: ignore
    def _to_py_deprecated(self) -> "service.ttypes.EchoResponse": ...  # type: ignore


class _fbthrift_compatible_with_WhisperException:
    pass


class WhisperException(_fbthrift_python_exceptions.GeneratedError, _fbthrift_compatible_with_WhisperException):
    message: _typing.Final[str] = ...
    def __init__(
        self, *,
        message: _typing.Optional[str]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[str]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "meta.example.thrift.service.thrift_mutable_types.WhisperException": ...  # type: ignore
    def _to_py3(self) -> "meta.example.thrift.service.types.WhisperException": ...  # type: ignore
    def _to_py_deprecated(self) -> "service.ttypes.WhisperException": ...  # type: ignore


class _fbthrift_EchoService_echo_args(_fbthrift_python_types.Struct):
    request: _typing.Final[_fbthrift_current_module.EchoRequest] = ...

    def __init__(
        self, *,
        request: _typing.Optional[_fbthrift_current_module.EchoRequest]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, _fbthrift_current_module.EchoRequest]]]: ...


class _fbthrift_EchoService_echo_result(_fbthrift_python_types.Struct):
    success: _typing.Final[_fbthrift_current_module.EchoResponse]
    ex: _typing.Final[_fbthrift_current_module.WhisperException]

    def __init__(
        self, *, success: _typing.Optional[_fbthrift_current_module.EchoResponse] = ..., ex: _typing.Optional[_fbthrift_current_module.WhisperException]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            _fbthrift_current_module.EchoResponse,
            _fbthrift_current_module.WhisperException,
        ]]]: ...
