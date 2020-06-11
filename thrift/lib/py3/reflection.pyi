# Copyright (c) Facebook, Inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from collections.abc import Mapping, Sequence, Set
from enum import Enum
from typing import Any, NamedTuple, Optional, Tuple, Type, Union, overload

from thrift.py3.client import Client
from thrift.py3.exceptions import Error
from thrift.py3.server import ServiceInterface
from thrift.py3.types import Struct
@overload
def inspect(cls: Union[Struct, Type[Struct], Error, Type[Error]]) -> StructSpec: ...
@overload
def inspect(
    cls: Union[ServiceInterface, Type[ServiceInterface], Client, Type[Client]]
) -> InterfaceSpec: ...
@overload
def inspect(cls: Union[Sequence[Any], Type[Sequence[Any]]]) -> ListSpec: ...
@overload
def inspect(cls: Union[Set[Any], Type[Set[Any]]]) -> SetSpec: ...
@overload
def inspect(cls: Union[Mapping[Any, Any], Type[Mapping[Any, Any]]]) -> MapSpec: ...
def inspectable(cls: Any) -> bool: ...

class NumberType(Enum):
    NOT_A_NUMBER = ...
    BYTE = ...
    I08 = ...
    I16 = ...
    I32 = ...
    I64 = ...
    FLOAT = ...
    DOUBLE = ...

class StructType(Enum):
    STRUCT: StructType = ...
    UNION: StructType = ...
    EXCEPTION: StructType = ...

class Qualifier(Enum):
    UNQUALIFIED: Qualifier = ...
    REQUIRED: Qualifier = ...
    OPTIONAL: Qualifier = ...

class StructSpec:
    name: str
    fields: Sequence[FieldSpec]
    kind: StructType
    annotations: Mapping[str, str] = {}
    def __init__(
        self,
        name: str,
        fields: Sequence[FieldSpec],
        kind: StructType,
        annotations: Mapping[str, str] = {},
    ): ...

class FieldSpec:
    name: str
    type: Type[Any]
    kind: NumberType
    qualifier: Qualifier
    default: Optional[Any]
    annotations: Mapping[str, str] = {}
    def __init__(
        self,
        name: str,
        type: Type[Any],
        kind: NumberType,
        qualifier: Qualifier,
        default: Optional[Any],
        annotations: Mapping[str, str] = {},
    ): ...

class ListSpec:
    value: Type[Any]
    kind: NumberType
    def __init__(self, value: Type[Any], kind: NumberType): ...

class SetSpec:
    value: Type[Any]
    kind: NumberType
    def __init__(self, value: Type[Any], kind: NumberType): ...

class MapSpec:
    key: Type[Any]
    key_kind: NumberType
    value: Type[Any]
    value_kind: NumberType
    def __init__(
        self,
        key: Type[Any],
        key_kind: NumberType,
        value: Type[Any],
        value_kind: NumberType,
    ): ...

class InterfaceSpec:
    name: str
    methods: Sequence[MethodSpec]
    annotations: Mapping[str, str] = {}
    def __init__(
        self,
        name: str,
        methods: Sequence[MethodSpec],
        annotations: Mapping[str, str] = {},
    ): ...

class MethodSpec:
    name: str
    arguments: Sequence[ArgumentSpec]
    result: Optional[Type[Any]]
    result_kind: NumberType
    exceptions: Sequence[Type[Any]] = []
    annotations: Mapping[str, str] = {}
    def __init__(
        self,
        name: str,
        arguments: Sequence[ArgumentSpec],
        result: Optional[Type[Any]],
        result_kind: NumberType,
        exceptions: Sequence[Type[Any]] = [],
        annotations: Mapping[str, str] = {},
    ): ...

class ArgumentSpec:
    name: str
    type: Type[Any]
    kind: NumberType
    annotations: Mapping[str, str] = {}
    def __init__(
        self,
        name: str,
        type: Type[Any],
        kind: NumberType,
        annotations: Mapping[str, str] = {},
    ): ...
