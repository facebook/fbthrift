#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from __future__ import annotations

import typing as _typing

import folly.iobuf as _fbthrift_iobuf
import foo.thrift_abstract_types as _fbthrift_python_abstract_types
import thrift.python.types as _fbthrift_python_types
import thrift.python.exceptions as _fbthrift_python_exceptions


class Fields(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.Fields):
    injected_field: _typing.Final[str] = ...
    injected_structured_annotation_field: _typing.Final[_typing.Optional[str]] = ...
    injected_unstructured_annotation_field: _typing.Final[_typing.Optional[str]] = ...
    def __init__(
        self, *,
        injected_field: _typing.Optional[str]=...,
        injected_structured_annotation_field: _typing.Optional[str]=...,
        injected_unstructured_annotation_field: _typing.Optional[str]=...
    ) -> None: ...

    def __call__(
        self, *,
        injected_field: _typing.Optional[str]=...,
        injected_structured_annotation_field: _typing.Optional[str]=...,
        injected_unstructured_annotation_field: _typing.Optional[str]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[str, str, str]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "foo.thrift_mutable_types.Fields": ...  # type: ignore
    def _to_py3(self) -> "foo.types.Fields": ...  # type: ignore
    def _to_py_deprecated(self) -> "foo.ttypes.Fields": ...  # type: ignore
_fbthrift_Fields = Fields
