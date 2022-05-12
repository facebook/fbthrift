#! /usr/bin/python3
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

import enum
import typing

import thrift.py3.reflection as py3_reflection
import thrift.py3.types as py3_types
import thrift.python.types as python_types
from thrift.Thrift import TType
from thrift.util import parse_struct_spec


T = typing.TypeVar("T")


def to_py_struct(
    cls: typing.Type[T], obj: typing.Union[py3_types.Struct, python_types.StructOrUnion]
) -> T:
    return _to_py_struct(cls, obj)


def _to_py_struct(
    cls: typing.Type[T], obj: typing.Union[py3_types.Struct, python_types.StructOrUnion]
) -> T:
    field_id_to_name = {}
    if isinstance(obj, py3_types.Struct):
        try:
            field_id_to_name = {
                field_spec.id: (
                    field_spec.annotations.get("py3.name") or field_spec.name
                )
                for field_spec in py3_reflection.inspect(obj).fields
            }
        except TypeError:
            pass
    elif isinstance(obj, python_types.StructOrUnion):
        # pyre-fixme[16]: `StructOrUnion` has no attribute `_fbthrift_SPEC`.
        field_id_to_name = {spec[0]: spec[2] for spec in obj._fbthrift_SPEC}

    # pyre-fixme[16]: `T` has no attribute `isUnion`.
    if cls.isUnion():
        if not isinstance(obj, py3_types.Union) and not isinstance(
            obj, python_types.Union
        ):
            raise TypeError("Source object is not an Union")
        return cls(
            **{
                field.name: _to_py_field(
                    field.type,
                    field.type_args,
                    getattr(obj, field_id_to_name.get(field.id, field.name)),
                )
                for field in parse_struct_spec(cls)
                if field_id_to_name.get(field.id, field.name) == obj.type.name
            }
        )
    else:
        return cls(
            **{
                field.name: _to_py_field(
                    field.type,
                    field.type_args,
                    getattr(obj, field_id_to_name.get(field.id, field.name)),
                )
                for field in parse_struct_spec(cls)
            }
        )


# pyre-fixme[3]: Return annotation cannot be `Any`.
def _to_py_field(
    field_type: TType,
    # pyre-fixme[2]: Parameter annotation cannot be `Any`.
    type_args: typing.Any,
    # pyre-fixme[2]: Parameter annotation cannot be `Any`.
    obj: typing.Any,
) -> typing.Any:
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
    # thrift-python Enums are subclasses of enum.Enum
    if isinstance(obj, py3_types.Enum) or isinstance(obj, enum.Enum):
        return obj.value
    return obj
