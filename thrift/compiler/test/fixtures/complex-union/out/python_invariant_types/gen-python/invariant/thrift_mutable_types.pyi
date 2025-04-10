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
import invariant.thrift_abstract_types as _fbthrift_python_abstract_types
import thrift.python.types as _fbthrift_python_types
import thrift.python.mutable_types as _fbthrift_python_mutable_types
import thrift.python.mutable_exceptions as _fbthrift_python_mutable_exceptions
import thrift.python.mutable_containers as _fbthrift_python_mutable_containers


class StructForInvariantTypes(_fbthrift_python_mutable_types.MutableStruct, _fbthrift_python_abstract_types.StructForInvariantTypes):
    @property
    def num(self) -> int: ...
    @num.setter
    def num(self, value: int) -> None: ...

    def __init__(
        self, *,
        num: _typing.Optional[int]=...
    ) -> None: ...

    def __call__(
        self, *,
        num: _typing.Optional[int]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int]]]: ...
    def _to_python(self) -> "invariant.thrift_types.StructForInvariantTypes": ...  # type: ignore
    def _to_mutable_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "invariant.types.StructForInvariantTypes": ...  # type: ignore
    def _to_py_deprecated(self) -> "invariant.ttypes.StructForInvariantTypes": ...  # type: ignore
_fbthrift_StructForInvariantTypes = StructForInvariantTypes

class UnionForInvariantTypes(_fbthrift_python_mutable_types.MutableStruct, _fbthrift_python_abstract_types.UnionForInvariantTypes):
    @property
    def num32(self) -> int: ...
    @num32.setter
    def num32(self, value: int) -> None: ...

    @property
    def num64(self) -> int: ...
    @num64.setter
    def num64(self, value: int) -> None: ...

    def __init__(
        self, *,
        num32: _typing.Optional[int]=...,
        num64: _typing.Optional[int]=...
    ) -> None: ...

    def __call__(
        self, *,
        num32: _typing.Optional[int]=...,
        num64: _typing.Optional[int]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int, int]]]: ...
    def _to_python(self) -> "invariant.thrift_types.UnionForInvariantTypes": ...  # type: ignore
    def _to_mutable_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "invariant.types.UnionForInvariantTypes": ...  # type: ignore
    def _to_py_deprecated(self) -> "invariant.ttypes.UnionForInvariantTypes": ...  # type: ignore
_fbthrift_UnionForInvariantTypes = UnionForInvariantTypes

class InvariantTypes(_fbthrift_python_mutable_types.MutableUnion, _fbthrift_python_abstract_types.InvariantTypes):
    @property
    def struct_map(self) -> _fbthrift_python_mutable_containers.MutableMap[_fbthrift_StructForInvariantTypes, int]: ...
    @struct_map.setter
    def struct_map(self, value: _fbthrift_python_mutable_containers.MutableMap[_fbthrift_StructForInvariantTypes, int] | _fbthrift_python_mutable_types._ThriftMapWrapper) -> None: ...

    @property
    def union_map(self) -> _fbthrift_python_mutable_containers.MutableMap[_fbthrift_UnionForInvariantTypes, int]: ...
    @union_map.setter
    def union_map(self, value: _fbthrift_python_mutable_containers.MutableMap[_fbthrift_UnionForInvariantTypes, int] | _fbthrift_python_mutable_types._ThriftMapWrapper) -> None: ...

    def __init__(
        self, *,
        struct_map: _typing.Optional[_fbthrift_python_mutable_containers.MutableMap[_fbthrift_StructForInvariantTypes, int] | _fbthrift_python_mutable_types._ThriftMapWrapper]=...,
        union_map: _typing.Optional[_fbthrift_python_mutable_containers.MutableMap[_fbthrift_UnionForInvariantTypes, int] | _fbthrift_python_mutable_types._ThriftMapWrapper]=...
    ) -> None: ...



    class FbThriftUnionFieldEnum(enum.Enum):
        EMPTY: InvariantTypes.FbThriftUnionFieldEnum = ...
        struct_map: InvariantTypes.FbThriftUnionFieldEnum = ...
        union_map: InvariantTypes.FbThriftUnionFieldEnum = ...

    fbthrift_current_value: _typing.Final[_typing.Union[None, _fbthrift_python_mutable_containers.MutableMap[_fbthrift_StructForInvariantTypes, int], _fbthrift_python_mutable_containers.MutableMap[_fbthrift_UnionForInvariantTypes, int]]]
    # pyre-ignore[15]: `fbthrift_current_field` overrides attribute defined in `_fbthrift_python_abstract_types.InvariantTypes` inconsistently. Type `InvariantTypes.FbThriftUnionFieldEnum` is not a subtype of the overridden attribute `_fbthrift_python_abstract_types.InvariantTypes.FbThriftUnionFieldEnum`
    fbthrift_current_field: _typing.Final[FbThriftUnionFieldEnum]
    def get_type(self) -> FbThriftUnionFieldEnum: ...
    def _to_python(self) -> "invariant.thrift_types.InvariantTypes": ...  # type: ignore
    def _to_mutable_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "invariant.types.InvariantTypes": ...  # type: ignore
    def _to_py_deprecated(self) -> "invariant.ttypes.InvariantTypes": ...  # type: ignore
_fbthrift_InvariantTypes = InvariantTypes
