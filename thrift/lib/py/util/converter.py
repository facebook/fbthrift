#! /usr/bin/python3
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

# pyre-unsafe

from typing import Any, Type, TypeVar

from thrift.py3.reflection import inspect
from thrift.py3.types import Enum, Struct
from thrift.Thrift import TType
from thrift.util import parse_struct_spec


T = TypeVar("T")


def to_py_struct(cls: Type[T], obj: Struct) -> T:
    return _to_py_struct(cls, obj)


def _to_py_struct(cls: Type[T], obj: Struct) -> T:
    try:
        field_id_to_py3_name = {
            field_spec.id: (field_spec.annotations.get("py3.name") or field_spec.name)
            for field_spec in inspect(obj).fields
        }
    except TypeError:
        field_id_to_py3_name = {}
    # pyre-fixme[16]: `T` has no attribute `isUnion`.
    if cls.isUnion():
        return cls(
            **{
                field.name: _to_py_field(
                    field.type,
                    field.type_args,
                    getattr(obj, field_id_to_py3_name.get(field.id, field.name)),
                )
                for field in parse_struct_spec(cls)
                # pyre-fixme[16]: `Struct` has no attribute `type`.
                if field_id_to_py3_name.get(field.id, field.name) == obj.type.name
            }
        )
    else:
        return cls(
            **{
                field.name: _to_py_field(
                    field.type,
                    field.type_args,
                    getattr(obj, field_id_to_py3_name.get(field.id, field.name)),
                )
                for field in parse_struct_spec(cls)
            }
        )


def _to_py_field(field_type: TType, type_args: Any, obj: Any) -> Any:
    if obj is None:
        return None
    if field_type == TType.STRUCT:
        return _to_py_struct(type_args[0], obj)
    if field_type == TType.LIST:
        return [_to_py_field(type_args[0], type_args[1], elem) for elem in obj]
    if field_type == TType.SET:
        return {_to_py_field(type_args[0], type_args[1], elem) for elem in obj}
    if field_type == TType.MAP:
        return {
            _to_py_field(type_args[0], type_args[1], k): _to_py_field(
                type_args[2], type_args[3], v
            )
            for k, v in obj.items()
        }
    if isinstance(obj, Enum):
        return obj.value
    return obj
