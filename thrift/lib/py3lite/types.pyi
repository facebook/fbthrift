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

import typing
from enum import Enum

from thrift.py3lite.exceptions import GeneratedError
from thrift.py3lite.serializer import Protocol

sT = typing.TypeVar("sT", bound=Struct)
uT = typing.TypeVar("uT", bound=Union)
eT = typing.TypeVar("eT", bound=Enum)

class TypeInfo:
    pass

class IntegerTypeInfo:
    pass

class StringTypeInfo:
    pass

typeinfo_bool: TypeInfo
typeinfo_byte: IntegerTypeInfo
typeinfo_i16: IntegerTypeInfo
typeinfo_i32: IntegerTypeInfo
typeinfo_i64: IntegerTypeInfo
typeinfo_double: TypeInfo
typeinfo_float: TypeInfo
typeinfo_string: StringTypeInfo
typeinfo_binary: TypeInfo

StructOrError = typing.Union[Struct, GeneratedError]

AnyTypeInfo = typing.Union[
    StructTypeInfo, ListTypeInfo, SetTypeInfo, MapTypeInfo, EnumTypeInfo
]

class ListTypeInfo:
    def __init__(self, val_info: AnyTypeInfo) -> None: ...

class SetTypeInfo:
    def __init__(self, val_info: AnyTypeInfo) -> None: ...

class MapTypeInfo:
    def __init__(self, key_info: AnyTypeInfo, val_info: AnyTypeInfo) -> None: ...

class StructTypeInfo:
    def __init__(self, klass: typing.Type[sT]) -> None: ...

class EnumTypeInfo:
    def __init__(self, klass: typing.Type[eT]) -> None: ...

class Struct(
    typing.Iterable[typing.Tuple[str, typing.Any]],
    typing.Hashable,
    metaclass=StructMeta,
):
    def __copy__(self: sT) -> sT: ...
    def __eq__(self) -> bool: ...
    def __hash__(self) -> int: ...
    def __lt__(self: sT, other: sT) -> bool: ...
    def __le__(self: sT, other: sT) -> bool: ...

class Union(
    typing.Hashable,
    metaclass=UnionMeta,
):
    def __eq__(self) -> bool: ...
    def __hash__(self) -> int: ...
    def __lt__(self: uT, other: uT) -> bool: ...
    def __le__(self: uT, other: uT) -> bool: ...
    def __bool__(self) -> bool: ...

class StructMeta(type): ...
class UnionMeta(type): ...

class BadEnum(typing.SupportsInt):
    enum: typing.Type[Enum]
    name: typing.Final[str]
    value: typing.Final[int]
    def __init__(self, the_enum: typing.Type[Enum], value: int) -> None: ...
    def __int__(self) -> int: ...

def fill_specs(*struct_types: StructTypeInfo) -> None: ...
def isset(struct: StructOrError) -> typing.Mapping[str, bool]: ...
def update_nested_field(
    obj: sT, path_to_values: typing.Mapping[str, typing.Any]
) -> sT: ...
