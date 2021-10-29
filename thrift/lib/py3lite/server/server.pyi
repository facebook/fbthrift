#!/usr/bin/env python3
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

import ipaddress
from types import TracebackType
from typing import (
    Any,
    Callable,
    ClassVar,
    Mapping,
    NamedTuple,
    Optional,
    Type,
    TypeVar,
    Union,
    overload,
)

from thrift.py3.server import ThriftServer

IPAddress = Union[ipaddress.IPv4Address, ipaddress.IPv6Address]
# pyre-fixme[24]: Generic type `os.PathLike` expects 1 type parameter.
Path = Union[str, bytes, os.PathLike]

class ServiceInterface:
    @staticmethod
    def service_name() -> bytes: ...
    # pyre-ignore[3]: it can return anything
    async def __aenter__(self) -> Any: ...
    async def __aexit__(
        self,
        exc_type: Optional[Type[BaseException]],
        exc_value: Optional[BaseException],
        traceback: Optional[TracebackType],
    ) -> Optional[bool]: ...

class Py3LiteServer(ThriftServer):
    def __init__(
        self,
        handler: ServiceInterface,
        port: int = 0,
        ip: Optional[Union[IPAddress, str]] = None,
        path: Optional[Path] = None,
    ) -> None: ...
