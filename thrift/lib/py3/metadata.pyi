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

from typing import Protocol, Type, Union, Optional, runtime_checkable, overload

from apache.thrift.metadata.types import (
    ThriftMetadata,
    ThriftStruct,
    ThriftStructType,
    ThriftUnionType,
    ThriftException,
    ThriftService,
    ThriftEnum,
    ThriftEnumType,
)
from thrift.py3.client import Client
from thrift.py3.exceptions import Error
from thrift.py3.server import ServiceInterface
from thrift.py3.types import Struct, Enum

class Metadata(Protocol):
    def getThriftModuleMetadata(self) -> ThriftMetadata: ...

@overload
def gen_metadata(cls: Metadata) -> ThriftMetadata: ...
@overload
def gen_metadata(cls: Union[Struct, Type[Struct]]) -> ThriftStruct: ...
@overload
def gen_metadata(cls: Union[Error, Type[Error]]) -> ThriftException: ...
@overload
def gen_metadata(
    cls: Union[ServiceInterface, Type[ServiceInterface], Client, Type[Client]]
) -> ThriftService: ...
@overload
def gen_metadata(cls: Union[Enum, Type[Enum]]) -> ThriftEnum: ...
