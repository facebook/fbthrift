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

# this is the generated pure Python code from Thrift IDL

import enum
import typing

import thrift.py3lite.types as _fbthrift_py3lite_types

class MyEnum(enum.Enum):
    ONE: MyEnum = ...
    TWO: MyEnum = ...

class IncludedStruct(_fbthrift_py3lite_types.Struct):
    intField: int
    listOfIntField: typing.Sequence[int]
    def __init__(
        self, *, intField: int = ..., listOfIntField: typing.Sequence[int] = ...
    ) -> None: ...
    def __call__(
        self,
        *,
        intField: typing.Optional[int] = ...,
        listOfIntField: typing.Optional[typing.Sequence[int]] = ...
    ) -> IncludedStruct: ...
    def __iter__(
        self,
    ) -> typing.Iterator[
        typing.Tuple[str, typing.Union[int, typing.Sequence[int]]]
    ]: ...
