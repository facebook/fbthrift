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

import enum
import types
import typing

from thrift.py3lite.types import Struct, Union

TAsyncClient = typing.TypeVar("TAsyncClient", bound="AsyncClient")
StructOrUnion = typing.TypeVar("StructOrUnion", bound=typing.Union[Struct, Union])

class AsyncClient:
    def __init__(self, service_name: str) -> None: ...
    async def __aenter__(self: TAsyncClient) -> TAsyncClient: ...
    async def __aexit__(
        self,
        exc_type: typing.Optional[typing.Type[BaseException]],
        exc_value: typing.Optional[BaseException],
        traceback: typing.Optional[types.TracebackType],
    ) -> None: ...
    async def _send_request(
        self,
        service_name: str,
        function_name: str,
        args: Struct,
        response_cls: typing.Optional[typing.Type[StructOrUnion]],
    ) -> StructOrUnion: ...
    def set_persistent_header(self, key: str, value: str) -> None: ...

class ClientType(enum.Enum):
    THRIFT_HEADER_CLIENT_TYPE: ClientType = ...
    THRIFT_ROCKET_CLIENT_TYPE: ClientType = ...
    THRIFT_HTTP_CLIENT_TYPE: ClientType = ...
