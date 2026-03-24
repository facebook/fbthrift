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

from typing import AsyncIterator

import folly.iobuf as _fbthrift_iobuf
from thrift.python.exceptions import GeneratedError
from thrift.python.mutable_exceptions import MutableGeneratedError
from thrift.python.mutable_types import MutableStruct
from thrift.python.protocol import Protocol
from thrift.python.types import Struct

class UserExceptionMeta:
    ex_class: type[GeneratedError | MutableGeneratedError]
    ex_field: str
    ex_name: str
    def __init__(
        self,
        ex_class: type[GeneratedError | MutableGeneratedError],
        ex_field: str,
        ex_name: str,
    ) -> None: ...

class CloseableGenerator(AsyncIterator[_fbthrift_iobuf.IOBuf]):
    def __init__(
        self,
        stream_generator: AsyncIterator[object],
        protocol: Protocol,
        return_struct_class: type[Struct | MutableStruct],
        user_exceptions: tuple[UserExceptionMeta, ...],
    ) -> None: ...
    def __aiter__(self) -> AsyncIterator[_fbthrift_iobuf.IOBuf]: ...
    async def __anext__(self) -> _fbthrift_iobuf.IOBuf: ...
    async def asend(self, value: None) -> _fbthrift_iobuf.IOBuf: ...
    async def athrow(
        self, typ: object, val: object = None, tb: object = None
    ) -> _fbthrift_iobuf.IOBuf: ...
    async def aclose(self) -> None: ...
