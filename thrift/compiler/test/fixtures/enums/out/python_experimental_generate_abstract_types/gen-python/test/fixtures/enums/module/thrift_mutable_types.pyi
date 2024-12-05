#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from __future__ import annotations


# EXPERIMENTAL - DO NOT USE !!!
# See `experimental_generate_mutable_types` documentation in thrift compiler

#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from __future__ import annotations

import typing as _typing

import test.fixtures.enums.module.thrift_mutable_types as _fbthrift_current_module
import folly.iobuf as _fbthrift_iobuf
import test.fixtures.enums.module.thrift_abstract_types as _fbthrift_python_abstract_types
import thrift.python.types as _fbthrift_python_types
import thrift.python.mutable_types as _fbthrift_python_mutable_types
import thrift.python.mutable_exceptions as _fbthrift_python_mutable_exceptions
import thrift.python.mutable_containers as _fbthrift_python_mutable_containers
from test.fixtures.enums.module.thrift_enums import _fbthrift_compatible_with_Metasyntactic
from test.fixtures.enums.module.thrift_enums import _fbthrift_compatible_with_MyEnum1
from test.fixtures.enums.module.thrift_enums import _fbthrift_compatible_with_MyEnum2
from test.fixtures.enums.module.thrift_enums import _fbthrift_compatible_with_MyEnum3
from test.fixtures.enums.module.thrift_enums import _fbthrift_compatible_with_MyEnum4
from test.fixtures.enums.module.thrift_enums import _fbthrift_compatible_with_MyBitmaskEnum1
from test.fixtures.enums.module.thrift_enums import _fbthrift_compatible_with_MyBitmaskEnum2

from test.fixtures.enums.module.thrift_enums import *


class _fbthrift_compatible_with_SomeStruct:
    pass


class SomeStruct(_fbthrift_python_mutable_types.MutableStruct, _fbthrift_compatible_with_SomeStruct, _fbthrift_python_abstract_types.SomeStruct):

    @property
    def reasonable(self) -> _fbthrift_current_module.Metasyntactic: ...
    @reasonable.setter
    def reasonable(self, value: _fbthrift_current_module.Metasyntactic): ...


    @property
    def fine(self) -> _fbthrift_current_module.Metasyntactic: ...
    @fine.setter
    def fine(self, value: _fbthrift_current_module.Metasyntactic): ...


    @property
    def questionable(self) -> _fbthrift_current_module.Metasyntactic: ...
    @questionable.setter
    def questionable(self, value: _fbthrift_current_module.Metasyntactic): ...


    @property
    def tags(self) -> _fbthrift_python_mutable_containers.MutableSet[int]: ...
    @tags.setter
    def tags(self, value: _fbthrift_python_mutable_containers.MutableSet[int] | _fbthrift_python_mutable_types._ThriftSetWrapper): ...

    def __init__(
        self, *,
        reasonable: _typing.Optional[_fbthrift_compatible_with_Metasyntactic]=...,
        fine: _typing.Optional[_fbthrift_compatible_with_Metasyntactic]=...,
        questionable: _typing.Optional[_fbthrift_compatible_with_Metasyntactic]=...,
        tags: _typing.Optional[_fbthrift_python_mutable_containers.MutableSet[int] | _fbthrift_python_mutable_types._ThriftSetWrapper]=...
    ) -> None: ...

    def __call__(
        self, *,
        reasonable: _typing.Optional[_fbthrift_compatible_with_Metasyntactic]=...,
        fine: _typing.Optional[_fbthrift_compatible_with_Metasyntactic]=...,
        questionable: _typing.Optional[_fbthrift_compatible_with_Metasyntactic]=...,
        tags: _typing.Optional[_fbthrift_python_mutable_containers.MutableSet[int] | _fbthrift_python_mutable_types._ThriftSetWrapper]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[_fbthrift_current_module.Metasyntactic, _fbthrift_current_module.Metasyntactic, _fbthrift_current_module.Metasyntactic, _fbthrift_python_mutable_containers.MutableSet[int]]]]: ...
    def _to_python(self) -> "test.fixtures.enums.module.thrift_types.SomeStruct": ...  # type: ignore
    def _to_mutable_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "test.fixtures.enums.module.types.SomeStruct": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.SomeStruct": ...  # type: ignore


class _fbthrift_compatible_with_MyStruct:
    pass


class MyStruct(_fbthrift_python_mutable_types.MutableStruct, _fbthrift_compatible_with_MyStruct, _fbthrift_python_abstract_types.MyStruct):

    @property
    def me2_3(self) -> _fbthrift_current_module.MyEnum2: ...
    @me2_3.setter
    def me2_3(self, value: _fbthrift_current_module.MyEnum2): ...


    @property
    def me3_n3(self) -> _fbthrift_current_module.MyEnum3: ...
    @me3_n3.setter
    def me3_n3(self, value: _fbthrift_current_module.MyEnum3): ...


    @property
    def me1_t1(self) -> _fbthrift_current_module.MyEnum1: ...
    @me1_t1.setter
    def me1_t1(self, value: _fbthrift_current_module.MyEnum1): ...


    @property
    def me1_t2(self) -> _fbthrift_current_module.MyEnum1: ...
    @me1_t2.setter
    def me1_t2(self, value: _fbthrift_current_module.MyEnum1): ...

    def __init__(
        self, *,
        me2_3: _typing.Optional[_fbthrift_compatible_with_MyEnum2]=...,
        me3_n3: _typing.Optional[_fbthrift_compatible_with_MyEnum3]=...,
        me1_t1: _typing.Optional[_fbthrift_compatible_with_MyEnum1]=...,
        me1_t2: _typing.Optional[_fbthrift_compatible_with_MyEnum1]=...
    ) -> None: ...

    def __call__(
        self, *,
        me2_3: _typing.Optional[_fbthrift_compatible_with_MyEnum2]=...,
        me3_n3: _typing.Optional[_fbthrift_compatible_with_MyEnum3]=...,
        me1_t1: _typing.Optional[_fbthrift_compatible_with_MyEnum1]=...,
        me1_t2: _typing.Optional[_fbthrift_compatible_with_MyEnum1]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[_fbthrift_current_module.MyEnum2, _fbthrift_current_module.MyEnum3, _fbthrift_current_module.MyEnum1, _fbthrift_current_module.MyEnum1]]]: ...
    def _to_python(self) -> "test.fixtures.enums.module.thrift_types.MyStruct": ...  # type: ignore
    def _to_mutable_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "test.fixtures.enums.module.types.MyStruct": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.MyStruct": ...  # type: ignore
