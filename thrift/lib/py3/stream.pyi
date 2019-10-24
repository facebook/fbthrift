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

from typing import Iterator, TypeVar

_T = TypeVar("_T")
sT = TypeVar("sT", bound=SemiStream)
rT = TypeVar("rT")
rsT = TypeVar("rsT", bound=ResponseAndSemiStream)

class SemiStream:
    """
    Base class for all SemiStream object
    """

    async def __aiter__(self: sT) -> Iterator[_T]: ...
    async def __anext__(self: sT) -> _T: ...

class ResponseAndSemiStream:
    def get_response(self: rsT) -> rT: ...
    def get_stream(self: rST, buffer_size: int) -> sT: ...
