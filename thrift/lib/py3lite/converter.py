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


import typing

from thrift.py3.serializer import (
    serialize_iobuf as py3_serialize,
    Protocol as py3_Protocol,
)
from thrift.py3.types import Struct as py3_Struct
from thrift.py3lite.serializer import deserialize, Protocol
from thrift.py3lite.types import Struct, Union

T = typing.TypeVar("T", bound=typing.Union[Struct, Union])


def to_py3lite_struct(cls: typing.Type[T], obj: py3_Struct) -> T:
    return deserialize(
        cls,
        py3_serialize(obj, protocol=py3_Protocol.BINARY),
        protocol=Protocol.BINARY,
    )
