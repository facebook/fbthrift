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


class Mixin1(_fbthrift_python_mutable_types.MutableStruct, _fbthrift_python_abstract_types.Mixin1):
    @property
    def field1(self) -> str: ...
    @field1.setter
    def field1(self, value: str) -> None: ...

    def __init__(
        self, *,
        field1: _typing.Optional[str]=...
    ) -> None: ...

    def __call__(
        self, *,
        field1: _typing.Optional[str]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[str]]]: ...
    def _to_python(self) -> "module.thrift_types.Mixin1": ...  # type: ignore
    def _to_mutable_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "module.types.Mixin1": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.Mixin1": ...  # type: ignore
_fbthrift_Mixin1 = Mixin1

class Mixin2(_fbthrift_python_mutable_types.MutableStruct, _fbthrift_python_abstract_types.Mixin2):
    @property
    def m1(self) -> _fbthrift_Mixin1: ...
    @m1.setter
    def m1(self, value: _fbthrift_Mixin1) -> None: ...

    @property
    def field2(self) -> _typing.Optional[str]: ...
    @field2.setter
    def field2(self, value: _typing.Optional[str]) -> None: ...

    def __init__(
        self, *,
        m1: _typing.Optional[Mixin1]=...,
        field2: _typing.Optional[str]=...
    ) -> None: ...

    def __call__(
        self, *,
        m1: _typing.Optional[Mixin1]=...,
        field2: _typing.Optional[str]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[_fbthrift_Mixin1, str]]]: ...
    def _to_python(self) -> "module.thrift_types.Mixin2": ...  # type: ignore
    def _to_mutable_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "module.types.Mixin2": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.Mixin2": ...  # type: ignore
_fbthrift_Mixin2 = Mixin2

class Mixin3Base(_fbthrift_python_mutable_types.MutableStruct, _fbthrift_python_abstract_types.Mixin3Base):
    @property
    def field3(self) -> str: ...
    @field3.setter
    def field3(self, value: str) -> None: ...

    def __init__(
        self, *,
        field3: _typing.Optional[str]=...
    ) -> None: ...

    def __call__(
        self, *,
        field3: _typing.Optional[str]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[str]]]: ...
    def _to_python(self) -> "module.thrift_types.Mixin3Base": ...  # type: ignore
    def _to_mutable_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "module.types.Mixin3Base": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.Mixin3Base": ...  # type: ignore
_fbthrift_Mixin3Base = Mixin3Base

class Foo(_fbthrift_python_mutable_types.MutableStruct, _fbthrift_python_abstract_types.Foo):
    @property
    def field4(self) -> str: ...
    @field4.setter
    def field4(self, value: str) -> None: ...

    @property
    def m2(self) -> _fbthrift_Mixin2: ...
    @m2.setter
    def m2(self, value: _fbthrift_Mixin2) -> None: ...

    @property
    def m3(self) -> _fbthrift_Mixin3Base: ...
    @m3.setter
    def m3(self, value: _fbthrift_Mixin3Base) -> None: ...

    def __init__(
        self, *,
        field4: _typing.Optional[str]=...,
        m2: _typing.Optional[Mixin2]=...,
        m3: _typing.Optional[Mixin3Base]=...
    ) -> None: ...

    def __call__(
        self, *,
        field4: _typing.Optional[str]=...,
        m2: _typing.Optional[Mixin2]=...,
        m3: _typing.Optional[Mixin3Base]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[str, _fbthrift_Mixin2, _fbthrift_Mixin3Base]]]: ...
    def _to_python(self) -> "module.thrift_types.Foo": ...  # type: ignore
    def _to_mutable_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "module.types.Foo": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.Foo": ...  # type: ignore
_fbthrift_Foo = Foo

Mixin3 = _fbthrift_Mixin3Base
