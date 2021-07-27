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

from typing import Any, Type

from thrift.py3.reflection import MapSpec, StructType, inspect, Qualifier
from thrift.py3.types import CompiledEnum, Container, Struct


def to_py3_struct(cls, obj):
    return _to_py3_struct(cls, obj)


def extract_name(field_spec):
    return field_spec.annotations.get("py3.name") or field_spec.name


def ignore_field(obj, field_spec):
    dft = field_spec.default
    if not (field_spec.qualifier == Qualifier.OPTIONAL and dft is not None):
        return False

    typ = field_spec.type
    val = getattr(obj, field_spec.name)
    casted = val if not issubclass(typ, CompiledEnum) else typ(val)
    return casted == dft

cdef object _to_py3_struct(object cls, object obj):
    struct_spec = inspect(cls)
    if struct_spec.kind == StructType.STRUCT:
        return cls(
            **{
                extract_name(field_spec): _to_py3_field(
                    field_spec.type, getattr(obj, field_spec.name)
                )
                for field_spec in struct_spec.fields
                if not ignore_field(obj, field_spec)
            }
        )
    elif struct_spec.kind == StructType.UNION:
        for field_spec in struct_spec.fields:
            try:
                value = getattr(obj, "get_" + field_spec.name)()
                field = _to_py3_field(field_spec.type, value)
                return cls(**{extract_name(field_spec): field})
            except AssertionError:
                pass
        return cls()
    else:
        raise NotImplementedError("Can not convert {}".format(struct_spec.kind))


cdef object _to_py3_field(object cls, object obj):
    if obj is None:
        return None
    if issubclass(cls, Struct):
        return _to_py3_struct(cls, obj)
    elif issubclass(cls, Container):
        container_spec = inspect(cls)
        if isinstance(container_spec, MapSpec):
            return {
                _to_py3_field(container_spec.key, k): _to_py3_field(
                    container_spec.value, v
                )
                for k, v in obj.items()
            }
        else:
            return [_to_py3_field(container_spec.value, elem) for elem in obj]
    elif issubclass(cls, CompiledEnum):
        return cls(obj)
    else:
        return obj
