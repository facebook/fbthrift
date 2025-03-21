# Copyright (c) Meta Platforms, Inc. and affiliates.
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

# pyre-strict

import typing

from folly.iobuf import IOBuf
from thrift.python.exceptions import GeneratedError
from thrift.python.types import Enum, StructOrUnion

PrimitiveType = typing.Union[bool, int, float, str, bytes, IOBuf, Enum]
TPrimitive = typing.TypeVar("TPrimitive", bound=PrimitiveType)

StructOrUnionOrException = typing.Union[GeneratedError, StructOrUnion]
SerializableType = typing.Union[StructOrUnionOrException, PrimitiveType]
TSerializable = typing.TypeVar("TSerializable", bound=SerializableType)
TKey = typing.TypeVar("TKey", bound=SerializableType)
TValue = typing.TypeVar("TValue", bound=SerializableType)

SerializableTypeOrContainers = typing.Union[
    SerializableType,
    typing.Sequence[TSerializable],
    typing.AbstractSet[TSerializable],
    typing.Mapping[TKey, TValue],
]
ObjWithUri = typing.Union[StructOrUnionOrException, Enum]
ClassWithUri = typing.Type[ObjWithUri]
