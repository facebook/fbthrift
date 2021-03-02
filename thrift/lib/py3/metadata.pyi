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

from enum import Enum
from typing import (
    Protocol,
    Type,
    Tuple,
    Union,
    Optional,
    Sequence,
    overload,
)

from apache.thrift.metadata.types import (
    ThriftType,
    ThriftMetadata,
    ThriftField,
    ThriftFunction,
    ThriftStruct,
    ThriftStructType,
    ThriftUnionType,
    ThriftException,
    ThriftService,
    ThriftEnum,
    ThriftEnumType,
    ThriftPrimitiveType,
    ThriftSetType,
    ThriftListType,
    ThriftMapType,
    ThriftTypedefType,
    ThriftSinkType,
    ThriftStreamType,
)
from thrift.py3.client import Client
from thrift.py3.exceptions import Error
from thrift.py3.server import ServiceInterface
from thrift.py3.types import Struct, Enum as ThriftEnumClass

class ThriftKind(Enum):
    PRIMITIVE: ThriftKind = ...
    LIST: ThriftKind = ...
    SET: ThriftKind = ...
    MAP: ThriftKind = ...
    ENUM: ThriftKind = ...
    STRUCT: ThriftKind = ...
    UNION: ThriftKind = ...
    TYPEDEF: ThriftKind = ...
    STREAM: ThriftKind = ...
    SINK: ThriftKind = ...

class Metadata(Protocol):
    def getThriftModuleMetadata(self) -> ThriftMetadata: ...

class ThriftTypeProxy(Protocol):
    thriftType: Union[
        ThriftPrimitiveType,
        ThriftSetType,
        ThriftListType,
        ThriftMapType,
        ThriftTypedefType,
        ThriftEnum,
        ThriftStruct,
        ThriftSinkType,
        ThriftStreamType,
    ]
    thriftMeta: ThriftMetadata
    kind: ThriftKind
    def as_primitive(self) -> ThriftPrimitiveType: ...
    def as_struct(self) -> ThriftStructProxy: ...
    def as_union(self) -> ThriftStructProxy: ...
    def as_enum(self) -> ThriftEnum: ...
    def as_list(self) -> ThriftListProxy: ...
    def as_set(self) -> ThriftSetProxy: ...
    def as_map(self) -> ThriftMapProxy: ...
    def as_typedef(self) -> ThriftTypeProxy: ...
    def as_stream(self) -> ThriftStreamProxy: ...
    def as_sink(self) -> ThriftSinkProxy: ...

class ThriftSetProxy(ThriftTypeProxy):
    thriftType: ThriftSetType
    valueType: ThriftTypeProxy

class ThriftListProxy(ThriftTypeProxy):
    thriftType: ThriftListType
    valueType: ThriftTypeProxy

class ThriftMapProxy(ThriftTypeProxy):
    thriftType: ThriftMapType
    valueType: ThriftTypeProxy
    keyType: ThriftTypeProxy

class ThriftTypedefProxy(ThriftTypeProxy):
    thriftType: ThriftTypedefType
    name: str
    underlyingType: ThriftTypeProxy

class ThriftSinkProxy(ThriftTypeProxy):
    thriftType: ThriftSinkType
    elemType: ThriftTypeProxy
    initialResponseType: ThriftTypeProxy

class ThriftStreamProxy(ThriftTypeProxy):
    thriftType: ThriftStreamType
    elemType: ThriftTypeProxy
    finalResponseType: ThriftTypeProxy
    initialResponseType: ThriftTypeProxy

class ThriftFieldProxy(Protocol):
    id: int
    name: str
    is_optional: bool
    type: ThriftTypeProxy
    thriftType: ThriftField
    thriftMeta: ThriftMetadata

class ThriftStructProxy(ThriftTypeProxy):
    thriftType: ThriftStruct
    thriftMeta: ThriftMetadata
    name: str
    fields: Sequence[ThriftFieldProxy]
    is_union: bool

class ThriftExceptionProxy(Protocol):
    name: str
    fields: Sequence[ThriftFieldProxy]
    thriftType: ThriftException
    thriftMeta: ThriftMetadata

class ThriftFunctionProxy(Protocol):
    name: str
    return_type: ThriftTypeProxy
    arguments: Sequence[ThriftFieldProxy]
    exceptions: Sequence[ThriftFieldProxy]
    is_oneway: bool
    thriftType: ThriftFunction
    thriftMeta: ThriftMetadata

class ThriftServiceProxy(Protocol):
    name: str
    functions: Sequence[ThriftFunctionProxy]
    parent: Optional[ThriftServiceProxy]
    thriftType: ThriftService
    thriftMeta: ThriftMetadata

@overload
def gen_metadata(cls: Metadata) -> ThriftMetadata: ...
@overload
def gen_metadata(cls: Union[Struct, Type[Struct]]) -> ThriftStructProxy: ...
@overload
def gen_metadata(cls: Union[Error, Type[Error]]) -> ThriftExceptionProxy: ...
@overload
def gen_metadata(
    cls: Union[ServiceInterface, Type[ServiceInterface], Client, Type[Client]]
) -> ThriftServiceProxy: ...
@overload
def gen_metadata(cls: Union[ThriftEnumClass, Type[ThriftEnumClass]]) -> ThriftEnum: ...
