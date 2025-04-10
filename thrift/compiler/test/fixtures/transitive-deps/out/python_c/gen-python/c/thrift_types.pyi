#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from __future__ import annotations

import typing as _typing

import folly.iobuf as _fbthrift_iobuf
import c.thrift_abstract_types as _fbthrift_python_abstract_types
import thrift.python.types as _fbthrift_python_types
import thrift.python.exceptions as _fbthrift_python_exceptions


class C(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.C):
    i: _typing.Final[int] = ...
    def __init__(
        self, *,
        i: _typing.Optional[int]=...
    ) -> None: ...

    def __call__(
        self, *,
        i: _typing.Optional[int]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "c.thrift_mutable_types.C": ...  # type: ignore
    def _to_py3(self) -> "c.types.C": ...  # type: ignore
    def _to_py_deprecated(self) -> "c.ttypes.C": ...  # type: ignore
_fbthrift_C = C

class E(_fbthrift_python_exceptions.GeneratedError, _fbthrift_python_abstract_types.E):
    def __init__(
        self,
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[None]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "c.thrift_mutable_types.E": ...  # type: ignore
    def _to_py3(self) -> "c.types.E": ...  # type: ignore
    def _to_py_deprecated(self) -> "c.ttypes.E": ...  # type: ignore
_fbthrift_E = E
