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

from abc import ABC
from typing import Generic, Type, TypeVar

from thrift.py3lite.client.async_client import AsyncClient
from thrift.py3lite.client.sync_client import SyncClient

TAsyncClient = TypeVar("TAsyncClient", bound=AsyncClient)
TSyncClient = TypeVar("TSyncClient", bound=SyncClient)


class Client(Generic[TAsyncClient, TSyncClient], ABC):
    """
    Base class of all thrift-py3lite clients.
    """

    Async: Type[TAsyncClient]
    Sync: Type[TSyncClient]
