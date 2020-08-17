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
from folly.iobuf import IOBuf
from types import MappingProxyType

from thrift.py3.exceptions cimport GeneratedError

__all__ = ['Struct', 'BadEnum', 'NOTSET', 'Union', 'Enum', 'Flag']


cdef __NotSet NOTSET = __NotSet.__new__(__NotSet)

# This isn't exposed to the module dict
Object = cython.fused_type(Struct, GeneratedError)


@cython.internal
@cython.auto_pickle(False)
cdef class StructMeta(type):
    """
    We set helper functions here since they can't possibly confict with field names
    """
    @staticmethod
    def isset(Object struct):
        return struct.__fbthrift_isset()


class _IsSet:
    __slots__ = ['__dict__', '__name__']

    def __init__(self, n, d):
        self.__name__ = n
        self.__dict__ = d

    def __repr__(self):
        args = ''.join(f', {name}={value}' for name, value in self.__dict__.items())
        return f'Struct.isset(<{self.__name__}>{args})'

cdef class Struct:
    """
    Base class for all thrift structs
    """
    cdef IOBuf _serialize(self, Protocol proto):
        return IOBuf(b'')

    cdef uint32_t _deserialize(self, const cIOBuf* buf, Protocol proto) except? 0:
        return 0

    cdef object __fbthrift_isset(self):
        raise TypeError(f"{type(self)} does not have concept of isset")


SetMetaClass(<PyTypeObject*> Struct, <PyTypeObject*> StructMeta)


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
