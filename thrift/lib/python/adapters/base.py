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

import abc
import typing

from thrift.python.types import StructOrUnion

OT = typing.TypeVar("Original")
AT = typing.TypeVar("Adapted")


class Adapter(typing.Generic[OT, AT], abc.ABC):
    """
    Base class of Python adapter.
    """

    """
    For Type Adapter, override (from|to)_thrift.
    """

    @classmethod
    def from_thrift(cls, original: OT) -> AT:
        raise NotImplementedError()

    @classmethod
    def to_thrift(cls, adapted: AT) -> OT:
        raise NotImplementedError()

    """
    For Field Adapter, override (from|to)_thrift_field.
    """

    @classmethod
    def from_thrift_field(cls, ori: OT, field_id: int, strct: StructOrUnion) -> AT:
        return cls.from_thrift(ori)

    @classmethod
    def to_thrift_field(cls, adapted: AT, field_id: int, strct: StructOrUnion) -> OT:
        return cls.to_thrift(adapted)
