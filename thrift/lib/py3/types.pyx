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

from collections.abc import Iterable, Mapping
import enum
import itertools
import warnings
cimport cython

from cpython.object cimport Py_LT, Py_EQ
from cython.operator cimport dereference as deref, postincrement as inc
from folly.cast cimport down_cast_ptr
from folly.iobuf import IOBuf
from types import MappingProxyType

from thrift.py3.exceptions cimport GeneratedError

__all__ = ['Struct', 'BadEnum', 'NOTSET', 'Union', 'Enum', 'Flag']


cdef __NotSet NOTSET = __NotSet.__new__(__NotSet)

# This isn't exposed to the module dict
Object = cython.fused_type(Struct, GeneratedError)



cdef list_compare(object first, object second, int op):
    """ Take either Py_EQ or Py_LT, everything else is derived """
    if not (isinstance(first, Iterable) and isinstance(second, Iterable)):
        if op == Py_EQ:
            return False
        else:
            return NotImplemented

    if op == Py_EQ:
        if len(first) != len(second):
            return False

    for x, y in zip(first, second):
        if x != y:
            if op == Py_LT:
                return x < y
            else:
                return False

    if op == Py_LT:
        return len(first) < len(second)
    return True


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
    def __hash__(self):
        if not self.__hash:
            self.__hash = hash(tuple(self))
        return self.__hash


@cython.auto_pickle(False)
cdef class List(Container):
    """
    Base class for all thrift lists
    """

    def __hash__(self):
        return super().__hash__()

    def __add__(self, other):
        return type(self)(itertools.chain(self, other))

    def __eq__(self, other):
        return list_compare(self, other, Py_EQ)

    def __ne__(self, other):
        return not list_compare(self, other, Py_EQ)

    def __lt__(self, other):
        return list_compare(self, other, Py_LT)

    def __gt__(self, other):
        return list_compare(other, self, Py_LT)

    def __le__(self, other):
        result = list_compare(other, self, Py_LT)
        return not result if result is not NotImplemented else NotImplemented

    def __ge__(self, other):
        result = list_compare(self, other, Py_LT)
        return not result if result is not NotImplemented else NotImplemented

    def __repr__(self):
        if not self:
            return 'i[]'
        return f'i[{", ".join(map(repr, self))}]'

    def __reduce__(self):
        return (type(self), (list(self), ))


@cython.auto_pickle(False)
cdef class Set(Container):
    """
    Base class for all thrift sets
    """

    def __reduce__(self):
        return (type(self), (set(self), ))

    def __repr__(self):
        if not self:
            return 'iset()'
        return f'i{{{", ".join(map(repr, self))}}}'

    def isdisjoint(self, other):
        return len(self & other) == 0

    def union(self, other):
        return self | other

    def intersection(self, other):
        return self & other

    def difference(self, other):
        return self - other

    def symmetric_difference(self, other):
        return self ^ other

    def issubset(self, other):
        return self <= other

    def issuperset(self, other):
        return self >= other


@cython.auto_pickle(False)
cdef class Map(Container):
    """
    Base class for all thrift maps
    """

    def __eq__(self, other):
        if not (isinstance(self, Mapping) and isinstance(other, Mapping)):
            return False
        if len(self) != len(other):
            return False

        for key in self:
            if key not in other:
                return False
            if other[key] != self[key]:
                return False

        return True

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        if not self.__hash:
            self.__hash = hash(tuple(self.items()))
        return self.__hash

    def __repr__(self):
        if not self:
            return 'i{}'
        return f'i{{{", ".join(map(lambda i: f"{repr(i[0])}: {repr(i[1])}", self.items()))}}}'

    def __reduce__(self):
        return (type(self), (dict(self), ))

    def keys(self):
        return self.__iter__()


@cython.auto_pickle(False)
cdef class EnumData:
    @staticmethod
    cdef EnumData create(cEnumData* ptr, py_type):
        cdef EnumData inst = EnumData.__new__(EnumData)
        inst._py_type = py_type
        inst._cpp_obj = unique_ptr[cEnumData](ptr)
        return inst

    cdef get_by_name(self, str name):
        cdef bytes name_bytes = name.encode("utf-8") # to keep the buffer alive
        cdef string_view name_sv = string_view(name_bytes)
        cdef pair[PyObjectPtr, cOptionalInt] r = self._cpp_obj.get().tryGetByName(name_sv)
        cdef PyObject* inst = r.first
        cdef optional[int] value = r.second
        if inst != NULL:
            return <object>inst
        if not value.has_value():
            raise AttributeError(f"'{self._py_type.__name__}' has no attribute '{name}'")
        return <object>self._add_to_cache(name, value.value())

    cdef get_by_value(self, int value):
        if value < -(1<<31) or value >= (1 << 31):
            self._value_error(value)
        cdef pair[PyObjectPtr, string_view] r = self._cpp_obj.get().tryGetByValue(value)
        cdef PyObject* inst = r.first
        cdef string_view name = r.second
        if inst != NULL:
            return <object>inst
        if name.data() == NULL:
            self._value_error(value)
        return <object>self._add_to_cache(sv_to_str(name), value)

    cdef PyObject* _add_to_cache(self, str name, int value) except *:
        new_inst = self._py_type.__new__(self._py_type, name, value, NOTSET)
        return self._cpp_obj.get().tryAddToCache(
            value,
            <PyObject*>new_inst
        )

    def get_all_names(self):
        cdef cEnumData* cpp_obj_ptr = self._cpp_obj.get()
        cdef cRange[const cStringPiece*] names = cpp_obj_ptr.getNames()
        cdef cStringPiece name
        for name in names:
            yield sv_to_str(cpp_obj_ptr.getPyName(string_view(name.data())))

    cdef int size(self):
        return self._cpp_obj.get().size()

    cdef void _value_error(self, int value) except *:
        raise ValueError(f"{value} is not a valid {self._py_type.__name__}")


@cython.auto_pickle(False)
cdef class EnumFlagsData(EnumData):

    @staticmethod
    cdef EnumFlagsData create(cEnumFlagsData* ptr, py_type):
        cdef EnumFlagsData inst = EnumFlagsData.__new__(EnumFlagsData)
        inst._py_type = py_type
        inst._cpp_obj = unique_ptr[cEnumData](ptr)
        return inst

    cdef get_by_value(self, int value):
        cdef cEnumFlagsData* cpp_obj_ptr = down_cast_ptr[
            cEnumFlagsData, cEnumData](self._cpp_obj.get())
        if value < 0:
            value = cpp_obj_ptr.convertNegativeValue(value)
        cdef pair[PyObjectPtr, string_view] r = cpp_obj_ptr.tryGetByValue(value)
        cdef PyObject* inst = r.first
        cdef string_view name = r.second
        if inst != NULL:
            return <object>inst
        if name.data() == NULL:
            self._value_error(value)
        if not name.empty():
            # it's not a derived value
            return <object>self._add_to_cache(sv_to_str(name), value)
        # it's a derived value
        new_inst = self._py_type.__new__(
            self._py_type,
            cpp_obj_ptr.getNameForDerivedValue(value).decode("utf-8"),
            value,
            NOTSET,
        )
        return <object>cpp_obj_ptr.tryAddToFlagValuesCache(value, <PyObject*>new_inst)

    cdef get_invert(self, uint32_t value):
        cdef cEnumFlagsData* cpp_obj_ptr = down_cast_ptr[
            cEnumFlagsData, cEnumData](self._cpp_obj.get())
        return self.get_by_value(cpp_obj_ptr.getInvertValue(value))

@cython.auto_pickle(False)
cdef class UnionTypeEnumData(EnumData):

    @staticmethod
    cdef UnionTypeEnumData create(cEnumData* ptr, py_type):
        cdef UnionTypeEnumData inst = UnionTypeEnumData.__new__(UnionTypeEnumData)
        inst._py_type = py_type
        inst._cpp_obj = unique_ptr[cEnumData](ptr)
        inst.__empty = py_type.__new__(py_type, "EMPTY", 0, NOTSET)
        return inst

    def get_all_names(self):
        yield "EMPTY"
        yield from EnumData.get_all_names(self)

    cdef get_by_name(self, str name):
        if name == "EMPTY":
            return self.__empty
        return EnumData.get_by_name(self, name)

    cdef get_by_value(self, int value):
        if value == 0:
            return self.__empty
        return EnumData.get_by_value(self, value)

    cdef int size(self):
        return EnumData.size(self) + 1  # for EMPTY


@cython.auto_pickle(False)
cdef class EnumMeta(type):
    def __get_by_name(cls, str name):
        return NotImplemented

    def __get_by_value(cls, int value):
        return NotImplemented

    def __get_all_names(cls):
        return NotImplemented

    def __call__(cls, value):
        if isinstance(value, cls):
            return value
        if not isinstance(value, int):
            raise ValueError(f"{repr(value)} is not a valid {cls.__name__}")
        return cls.__get_by_value(value)

    def __getitem__(cls, name):
        if not isinstance(name, str):
            raise KeyError(name)
        try:
            return cls.__get_by_name(str(name))
        except AttributeError:
            raise KeyError(name)

    def __getattribute__(cls, str name not None):
        if name.startswith("__") or name == "mro":
            return super().__getattribute__(name)
        return cls.__get_by_name(name)

    def __iter__(cls):
        for name in cls.__get_all_names():
            yield cls.__get_by_name(name)

    def __reversed__(cls):
        return reversed(iter(cls))

    def __contains__(cls, item):
        if not isinstance(item, cls):
            return False
        return item in cls.__iter__()

    def __len__(cls):
        return NotImplemented

    @property
    def __members__(cls):
        return MappingProxyType({inst.name: inst for inst in cls.__iter__()})

    def __dir__(cls):
        return ['__class__', '__doc__', '__members__', '__module__'] + [name for name in cls.__get_all_names()]


@cython.auto_pickle(False)
cdef class CompiledEnum:
    """
    Base class for all thrift Enum
    """
    def __cinit__(self, name, value, __NotSet guard = None):
        if guard is not NOTSET:
            raise TypeError('__new__ is disabled in the interest of type-safety')
        self.name = name
        self.value = value
        self.__hash = hash(name)
        self.__str = f"{type(self).__name__}.{name}"
        self.__repr = f"<{self.__str}: {value}>"

    cdef get_by_name(self, str name):
        return NotImplemented

    def __getattribute__(self, str name not None):
        if name.startswith("__") or name in ("name", "value"):
            return super().__getattribute__(name)
        return self.get_by_name(name)

    def __repr__(self):
        return self.__repr

    def __str__(self):
        return self.__str

    def __int__(self):
        return self.value

    def __hash__(self):
        return self.__hash

    def __reduce__(self):
        return type(self), (self.value,)

    def __eq__(self, other):
        if type(other) is not type(self):
            warnings.warn(f"comparison not supported between instances of { type(self) } and {type(other)}", RuntimeWarning, stacklevel=2)
            return False
        return self is other

Enum = CompiledEnum
# I wanted to call the base class Enum, but there is a cython bug
# See https://github.com/cython/cython/issues/2474
# Will move when the bug is fixed


@cython.auto_pickle(False)
cdef class Flag(CompiledEnum):
    """
    Base class for all thrift Flag
    """
    def __contains__(self, other):
        if type(other) is not type(self):
            return NotImplemented
        return other.value & self.value == other.value

    def __bool__(self):
        return bool(self.value)

    def __or__(self, other):
        cls = type(self)
        if type(other) is not cls:
            return NotImplemented
        return cls(self.value | other.value)

    def __and__(self, other):
        cls = type(self)
        if type(other) is not cls:
            return NotImplemented
        return cls(self.value & other.value)

    def __xor__(self, other):
        cls = type(self)
        if type(other) is not cls:
            return NotImplemented
        return cls(self.value ^ other.value)

    def __invert__(self):
        return NotImplemented


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
