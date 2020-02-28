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
cimport cython
from collections import namedtuple
from folly.iobuf import IOBuf
from types import MappingProxyType

__all__ = ['NumberType', 'Struct', 'BadEnum', 'NOTSET', 'Union', 'Enum', 'Flag']

class NumberType(enum.Enum):
    NONE = 0
    BYTE = 1
    I08 = 1
    I16 = 2
    I32 = 3
    I64 = 4
    FLOAT = 5
    DOUBLE = 6

class NOTSETTYPE(enum.Enum):
    token = 0

NOTSET = NOTSETTYPE.token


cdef class Struct:
    """
    Base class for all thrift structs
    """
    cdef IOBuf _serialize(self, proto):
        return IOBuf(b'')

    cdef uint32_t _deserialize(self, const cIOBuf* buf, proto) except? 0:
        return 0


cdef class Union(Struct):
    """
    Base class for all thrift Unions
    """
    pass


@cython.auto_pickle(False)
cdef class Container:
    """
    Base class for all thrift containers
    """
    pass


@cython.auto_pickle(False)
cdef class CompiledEnum:
    """
    Base class for all thrift Enum
    """
    pass

Enum = CompiledEnum
# I wanted to call the base class Enum, but there is a cython bug
# See https://github.com/cython/cython/issues/2474
# Will move when the bug is fixed


@cython.auto_pickle(False)
cdef class Flag(CompiledEnum):
    """
    Base class for all thrift Flag
    """
    pass


cdef class BadEnum:
    """
    This represents a BadEnum value from thrift.
    So an out of date thrift definition or a default value that is not
    in the enum
    """

    def __init__(self, the_enum, value):
        self._enum = the_enum
        self.value = value
        self.name = '#INVALID#'

    def __repr__(self):
        return f'<{self.enum.__name__}.{self.name}: {self.value}>'

    def __int__(self):
        return self.value

    @property
    def enum(self):
        return self._enum

    def __reduce__(self):
        return BadEnum, (self._enum, self.value)

    def __hash__(self):
        return hash((self._enum, self.value))

    def __eq__(self, other):
        if not isinstance(other, BadEnum):
            return False
        cdef BadEnum cother = <BadEnum>other
        return (self._enum, self.value) == (cother._enum, cother.value)

    def __ne__(self, other):
        return not(self == other)


cdef translate_cpp_enum_to_python(object EnumClass, int value):
    try:
        return EnumClass(value)
    except ValueError:
        return BadEnum(EnumClass, value)


class StructType(enum.Enum):
  STRUCT = 1
  UNION = 2
  EXCEPTION = 3


class Qualifier(enum.Enum):
  NONE = 1
  REQUIRED = 2
  OPTIONAL = 3


StructSpec = namedtuple('StructSpec', ['name', 'fields', 'kind', 'annotations'])
FieldSpec = namedtuple('FieldSpec', ['name', 'type', 'kind', 'qualifier', 'default', 'annotations'])
ListSpec = namedtuple('ListSpec', ['value', 'kind'])
SetSpec = namedtuple('SetSpec', ['value', 'kind'])
MapSpec = namedtuple('MapSpec', [ 'key', 'key_kind', 'value', 'value_kind'])
