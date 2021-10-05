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

from collections import defaultdict
from types import MappingProxyType

from libcpp.utility cimport move as cmove
from libcpp.memory cimport make_unique
from cpython cimport bool as pbool, int as pint, float as pfloat
from cpython.tuple cimport PyTuple_New, PyTuple_SET_ITEM, PyTuple_GET_ITEM, PyTuple_Check
from cpython.ref cimport Py_INCREF, Py_DECREF
from cython.operator cimport dereference as deref

import copy
import cython
import enum
import functools

from thrift.py3lite.exceptions cimport GeneratedError
from thrift.py3lite.exceptions import GeneratedErrorMeta
from thrift.py3lite.serializer cimport cserialize, cdeserialize


cdef class TypeInfo:
    @staticmethod
    cdef create(const cTypeInfo& cpp_obj, pytypes):
        cdef TypeInfo inst = TypeInfo.__new__(TypeInfo)
        inst.cpp_obj = &cpp_obj
        inst.pytypes = pytypes
        return inst

    # validate and convert to format serializer may understand
    def to_internal_data(self, object value):
        if not isinstance(value, self.pytypes):
            raise TypeError(f'value {value} is not a {self.pytypes !r}.')
        return value

    # convert deserialized data to user format
    def to_python_value(self, object value):
        return value


cdef class IntegerTypeInfo:
    @staticmethod
    cdef create(const cTypeInfo& cpp_obj, min_value, max_value):
        cdef IntegerTypeInfo inst = IntegerTypeInfo.__new__(IntegerTypeInfo)
        inst.cpp_obj = &cpp_obj
        inst.min_value = min_value
        inst.max_value = max_value
        return inst

    # validate and convert to format serializer may understand
    def to_internal_data(self, object value):
        if not isinstance(value, pint):
            raise TypeError(f"value {value} is not a <class 'int'>.")
        cdef int64_t cvalue = value
        if cvalue > self.max_value or cvalue < self.min_value:
            raise OverflowError()
        return value

    # convert deserialized data to user format
    def to_python_value(self, object value):
        return value



cdef class StringTypeInfo:
    @staticmethod
    cdef create(const cTypeInfo& cpp_obj):
        cdef StringTypeInfo inst = StringTypeInfo.__new__(StringTypeInfo)
        inst.cpp_obj = &cpp_obj
        return inst

    # validate and convert to format serializer may understand
    def to_internal_data(self, str value):
        return value.encode("UTF-8")

    # convert deserialized data to user format
    def to_python_value(self, object value):
        return value.decode("UTF-8")


typeinfo_bool = TypeInfo.create(boolTypeInfo, (pbool,))
typeinfo_byte = IntegerTypeInfo.create(byteTypeInfo, -128, 127)
typeinfo_i16 = IntegerTypeInfo.create(i16TypeInfo, -1<<15, (1<<15)-1)
typeinfo_i32 = IntegerTypeInfo.create(i32TypeInfo, -1<<31, (1<<31)-1)
typeinfo_i64 = IntegerTypeInfo.create(i64TypeInfo, -1<<63, (1<<63)-1)
typeinfo_double = TypeInfo.create(doubleTypeInfo, (pfloat, pint))
typeinfo_float = TypeInfo.create(floatTypeInfo, (pfloat, pint))
typeinfo_string = StringTypeInfo.create(stringTypeInfo)
typeinfo_binary = TypeInfo.create(stringTypeInfo, (bytes,))


StructOrError = cython.fused_type(Struct, GeneratedError)


cdef class StructInfo:
    def __cinit__(self, name, fields):
        cname = name.encode("utf-8")
        self.fields = fields
        self.cpp_obj = make_unique[cDynamicStructInfo](
            <const char*>cname,
            len(fields),
            False,
        )
        self.type_infos = []
        self.name_to_index = {}

    cdef void fill(self) except *:
        cdef cDynamicStructInfo* info_ptr = self.cpp_obj.get()
        for idx, (id, is_unqualified, name, type_info, _) in enumerate(self.fields):
            # type_info can be a lambda function so types with dependencies
            # won't need to be defined in order
            if callable(type_info):
                type_info = type_info()
            self.type_infos.append(type_info)
            self.name_to_index[name] = idx
            info_ptr.addFieldInfo(
                id, is_unqualified, name.encode("utf-8"), getCTypeInfo(type_info)
            )

    cdef void store_field_values(self) except *:
        cdef cDynamicStructInfo* info_ptr = self.cpp_obj.get()
        for idx, field in enumerate(self.fields):
            default_value = field[4]
            if default_value is None:
                continue
            if callable(default_value):
                default_value = default_value()
            default_value = self.type_infos[idx].to_internal_data(default_value)
            info_ptr.addFieldValue(idx, default_value)


cdef class UnionInfo:
    def __cinit__(self, name, fields):
        cname = name.encode("utf-8")
        self.fields = fields
        self.cpp_obj = make_unique[cDynamicStructInfo](
            <const char*>cname,
            len(fields),
            True,
        )
        self.type_infos = {}
        self.name_to_index = {}

    cdef void fill(self) except *:
        cdef cDynamicStructInfo* info_ptr = self.cpp_obj.get()
        for idx, (id, is_unqualified, name, type_info, _) in enumerate(self.fields):
            # type_info can be a lambda function so types with dependencies
            # won't need to be defined in order
            if callable(type_info):
                type_info = type_info()
            self.type_infos[id] = type_info
            info_ptr.addFieldInfo(
                id, is_unqualified, name.encode("utf-8"), getCTypeInfo(type_info)
            )


cdef const cTypeInfo* getCTypeInfo(type_info):
        if isinstance(type_info, TypeInfo):
            return (<TypeInfo>type_info).cpp_obj
        if isinstance(type_info, StringTypeInfo):
            return (<StringTypeInfo>type_info).cpp_obj
        if isinstance(type_info, IntegerTypeInfo):
            return (<IntegerTypeInfo>type_info).cpp_obj
        if isinstance(type_info, StructTypeInfo):
            return (<StructTypeInfo>type_info).get()
        if isinstance(type_info, ListTypeInfo):
            return (<ListTypeInfo>type_info).get()
        if isinstance(type_info, SetTypeInfo):
            return (<SetTypeInfo>type_info).get()
        if isinstance(type_info, MapTypeInfo):
            return (<MapTypeInfo>type_info).get()
        if isinstance(type_info, EnumTypeInfo):
            return &i32TypeInfo


cdef class ListTypeInfo:
    def __cinit__(self, val_info):
        self.val_info = val_info
        self.cpp_obj = make_unique[cListTypeInfo](getCTypeInfo(val_info))

    cdef const cTypeInfo* get(self):
        return self.cpp_obj.get().get()

    # validate and convert to format serializer may understand
    def to_internal_data(self, value):
        return tuple(self.val_info.to_internal_data(v) for v in value)

    # convert deserialized data to user format
    def to_python_value(self, object value):
        return tuple(self.val_info.to_python_value(v) for v in value)


cdef class SetTypeInfo:
    def __cinit__(self, val_info):
        self.val_info = val_info
        self.cpp_obj = make_unique[cSetTypeInfo](getCTypeInfo(val_info))

    cdef const cTypeInfo* get(self):
        return self.cpp_obj.get().get()

    # validate and convert to format serializer may understand
    def to_internal_data(self, value):
        return frozenset(self.val_info.to_internal_data(v) for v in value)

    # convert deserialized data to user format
    def to_python_value(self, object value):
        return frozenset(self.val_info.to_python_value(v) for v in value)


cdef class MapTypeInfo:
    def __cinit__(self, key_info, val_info):
        self.key_info = key_info
        self.val_info = val_info
        self.cpp_obj = make_unique[cMapTypeInfo](
            getCTypeInfo(key_info),
            getCTypeInfo(val_info),
        )

    cdef const cTypeInfo* get(self):
        return self.cpp_obj.get().get()

    # validate and convert to format serializer may understand
    def to_internal_data(self, value):
        return tuple(
            (self.key_info.to_internal_data(k), self.val_info.to_internal_data(v)) for k, v in value.items()
        )

    # convert deserialized data to user format
    def to_python_value(self, object value):
        return MappingProxyType({
            self.key_info.to_python_value(k):
            self.val_info.to_python_value(v)
            for k, v in value
        })


cdef class StructTypeInfo:
    def __cinit__(self, klass):
        self._class = klass
        info = klass._fbthrift_struct_info
        cdef cDynamicStructInfo* c_struct_info
        if isinstance(info, UnionInfo):
            self.is_union = True
            c_struct_info = (<UnionInfo>info).cpp_obj.get()
        else:
            self.is_union = False
            c_struct_info = (<StructInfo>info).cpp_obj.get()
        self.cpp_obj = createStructTypeInfo(
            deref(c_struct_info)
        )

    cdef const cTypeInfo* get(self):
        return &self.cpp_obj

    # validate and convert to format serializer may understand
    def to_internal_data(self, value):
        if not isinstance(value, self._class):
            raise TypeError(f"value {value} is not a {self._class !r}.")
        return (<Struct>value)._fbthrift_data

    # convert deserialized data to user format
    def to_python_value(self, object value):
        return self._class._fbthrift_create(value)


cdef class EnumTypeInfo:
    def __cinit__(self, klass):
        self._class = klass

    # validate and convert to format serializer may understand
    def to_internal_data(self, value):
        if not isinstance(value, self._class):
            raise TypeError(f"value {value} is not '{self._class}'.")
        return value._value_

    # convert deserialized data to user format
    def to_python_value(self, object value):
        try:
            return self._class(value)
        except ValueError:
            return BadEnum(self._class, value)


cdef void set_struct_field(tuple struct_tuple, int16_t index, value) except *:
    setStructIsset(struct_tuple, index, 1)
    old_value = struct_tuple[index + 1]
    Py_INCREF(value)
    PyTuple_SET_ITEM(struct_tuple, index + 1, value)
    Py_DECREF(old_value)


cdef class Struct:
    def __cinit__(self):
        cdef StructInfo info = self._fbthrift_struct_info
        self._fbthrift_data = createStructTuple(
            info.cpp_obj.get().getStructInfo()
        )

    def __init__(self, **kwargs):
        cdef StructInfo info = self._fbthrift_struct_info
        for name, value in kwargs.items():
            index = info.name_to_index.get(name)
            if index is None:
                raise TypeError(f"__init__() got an unexpected keyword argument '{name}'")
            if value is None:
                continue
            set_struct_field(
                self._fbthrift_data,
                index,
                info.type_infos[index].to_internal_data(value),
            )

    def __call__(self, **kwargs):
        if not kwargs:
            return self
        cdef StructInfo info = self._fbthrift_struct_info
        tp = type(self)
        cdef Struct new_inst = tp.__new__(tp)
        not_found = object()
        for name, index in info.name_to_index.items():
            value = kwargs.pop(name, not_found)
            if value is None:  # reset to default value, no change needed
                continue
            if value is not_found:  # copy the old value if needed
                if self._fbthrift_data[0][index] == 0: # old field not set, so keep default
                    continue
                value_to_copy = self._fbthrift_data[index + 1]
            else:  # new assigned value
                value_to_copy = info.type_infos[index].to_internal_data(value)
            set_struct_field(new_inst._fbthrift_data, index, value_to_copy)
        if kwargs:
            raise TypeError(f"__call__() got an expected keyword argument '{kwargs.keys()[0]}'")
        return new_inst

    def __copy__(Struct self):
        # deep copy the instance
        return self._fbthrift_create(copy.deepcopy(self._fbthrift_data))

    def __eq__(Struct self, other):
        if type(other) != type(self):
            return False
        cdef StructInfo info = self._fbthrift_struct_info
        for name in info.name_to_index:
            if getattr(self, name) != getattr(other, name):
                return False
        return True

    def __lt__(self, other):
        if type(self) != type(other):
            return NotImplemented
        ret = (<Struct>self)._fbthrift_data[1:] < (<Struct>other)._fbthrift_data[1:]
        return ret

    def __le__(self, other):
        return self < other or self == other

    def __hash__(Struct self):
        return hash(self._fbthrift_data)

    def __iter__(self):
        cdef StructInfo info = self._fbthrift_struct_info
        for name in info.name_to_index:
            yield name, getattr(self, name)

    cdef folly.iobuf.IOBuf _serialize(self, Protocol proto):
        cdef StructInfo info = self._fbthrift_struct_info
        return folly.iobuf.from_unique_ptr(
            cmove(cserialize(deref(info.cpp_obj), self._fbthrift_data, proto))
        )

    cdef uint32_t _deserialize(self, folly.iobuf.IOBuf buf, Protocol proto) except? 0:
        cdef StructInfo info = self._fbthrift_struct_info
        return cdeserialize(deref(info.cpp_obj), buf._this, self._fbthrift_data, proto)

    cdef _fbthrift_get_field_value(self, int16_t index):
        data = self._fbthrift_data[index + 1]
        if data is None:
            return
        cdef StructInfo info = self._fbthrift_struct_info
        return info.type_infos[index].to_python_value(data)


    @classmethod
    def _fbthrift_create(cls, data):
        cdef Struct inst = cls.__new__(cls)
        inst._fbthrift_data = data
        return inst


cdef class Union:
    def __cinit__(self):
        self._fbthrift_data = createUnionTuple()

    def __init__(self, **kwargs):
        if not kwargs:
            self._fbthrift_load_cache()
            return
        if len(kwargs) != 1:
            raise TypeError("__init__() of a union may only take one keyword argument")
        for name, value in kwargs.items():
            break
        try:
            tpe = self.Type[name]
        except KeyError:
            raise TypeError(f"__init__() got an unexpected keyword argument '{name}'")
        self._fbthrift_update_type_value(tpe.value, value)

    @classmethod
    def _fbthrift_create(cls, data):
        cdef Union inst = cls.__new__(cls)
        inst._fbthrift_data = data
        inst._fbthrift_load_cache()
        return inst

    cdef void _fbthrift_update_type_value(self, type_value, value) except *:
        Py_INCREF(type_value)
        old_type_value = self._fbthrift_data[0]
        PyTuple_SET_ITEM(self._fbthrift_data, 0, type_value)
        Py_DECREF(old_type_value)
        old_value = self._fbthrift_data[1]
        value = (<UnionInfo>self._fbthrift_struct_info).type_infos[type_value].to_internal_data(value)
        Py_INCREF(value)
        PyTuple_SET_ITEM(self._fbthrift_data, 1, value)
        Py_DECREF(old_value)
        self._fbthrift_load_cache()

    cdef void _fbthrift_load_cache(self) except *:
        self.type = self.Type(self._fbthrift_data[0])
        val = self._fbthrift_data[1]
        if val is None:
            self.value = None
            return
        cdef UnionInfo info = self._fbthrift_struct_info
        self.value = info.type_infos[self._fbthrift_data[0]].to_python_value(val)

    cdef folly.iobuf.IOBuf _serialize(self, Protocol proto):
        cdef UnionInfo info = self._fbthrift_struct_info
        return folly.iobuf.from_unique_ptr(
            cmove(cserialize(deref(info.cpp_obj), self._fbthrift_data, proto))
        )

    cdef uint32_t _deserialize(self, folly.iobuf.IOBuf buf, Protocol proto) except? 0:
        cdef UnionInfo info = self._fbthrift_struct_info
        cdef uint32_t size = cdeserialize(deref(info.cpp_obj), buf._this, self._fbthrift_data, proto)
        self._fbthrift_load_cache()
        return size

    cdef _fbthrift_get_field_value(self, int16_t index):
        if self.type.value != index:
            raise TypeError(f'Union contains a value of type {self.type.name}, not {self.Type(index).name}')
        return self.value

    def get_type(self):
        return self.type

    @classmethod
    def fromValue(cls, value):
        cdef Union inst = cls.__new__(cls)
        if value is None:
            return inst
        cdef UnionInfo info = cls._fbthrift_struct_info
        for type_value, typeinfo in info.type_infos.items():
            try:
                value = typeinfo.to_internal_data(value)
            except (TypeError, OverflowError):
                continue
            else:
                inst._fbthrift_update_type_value(type_value, value)
                break
        return inst

    def __copy__(Union self):
        # deep copy the instance
        return self._fbthrift_create(copy.deepcopy(self._fbthrift_data))

    def __eq__(Union self, other):
        if type(other) != type(self):
            return False
        return self._fbthrift_data == (<Union>other)._fbthrift_data

    def __lt__(self, other):
        if type(self) != type(other):
            return NotImplemented
        ret = (<Union>self)._fbthrift_data < (<Union>other)._fbthrift_data
        return ret

    def __le__(self, other):
        return self < other or self == other

    def __hash__(self):
        return hash(self._fbthrift_data)

    def __repr__(self):
        return f"{type(self).__name__}(type={self.type.name}, value={self.value!r})"

    def __bool__(self):
        return self.type.value != 0


cdef make_fget_struct(i):
    return functools.cached_property(lambda self: (<Struct>self)._fbthrift_get_field_value(i))

cdef make_fget_union(type_value):
    return property(lambda self: (<Union>self)._fbthrift_get_field_value(type_value))


class StructMeta(type):
    def __new__(cls, name, bases, dct):
        fields = dct.pop('_fbthrift_SPEC')
        num_fields = len(fields)
        dct["_fbthrift_struct_info"] = StructInfo(name, fields)
        for i, f in enumerate(fields):
            dct[f[2]] = make_fget_struct(i)
        return super().__new__(cls, name, (Struct,), dct)

    def _fbthrift_fill_spec(cls):
        (<StructInfo>cls._fbthrift_struct_info).fill()

    def _fbthrift_store_field_values(cls):
        (<StructInfo>cls._fbthrift_struct_info).store_field_values()



def gen_enum(fields):
    yield ("EMPTY", 0)
    for f in fields:
        yield (f[2], f[0])


class UnionMeta(type):
    def __new__(cls, name, bases, dct):
        fields = dct.pop('_fbthrift_SPEC')
        num_fields = len(fields)
        dct["_fbthrift_struct_info"] = UnionInfo(name, fields)
        dct["Type"] = enum.Enum(name, gen_enum(fields))
        for f in fields:
            dct[f[2]] = make_fget_union(f[0])
        return super().__new__(cls, name, (Union,), dct)

    def _fbthrift_fill_spec(cls):
        (<UnionInfo>cls._fbthrift_struct_info).fill()


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

# To support dependent classes not defined in order.
# If struct A has a field of type struct B, but the generated class A is
# defined before B, we are not able to populate the specs for A as part of the
# class creation.
# We will create all the classes first then call fill_specs after that so
# dependancies can be properly solved.
def fill_specs(*struct_types):
    for cls in struct_types:
        cls._fbthrift_fill_spec()

    for cls in struct_types:
        if isinstance(cls, (StructMeta, GeneratedErrorMeta)):
            cls._fbthrift_store_field_values()


def isset(StructOrError struct):
    cdef StructInfo info = struct._fbthrift_struct_info
    isset_bytes = struct._fbthrift_data[0]
    return {
        name: bool(isset_bytes[index])
        for name, index in info.name_to_index.items()
    }


def update_nested_field(Struct obj, path_to_values):
    # There is some optimization opportunity here for cases like this:
    # { "a.b.c": foo, "a.b.d": var }
    try:
        obj = _fbthrift_struct_update_nested_field(
            obj,
            [(p.split("."), v) for p, v in path_to_values.items()]
        )
        return obj
    except (AttributeError, TypeError) as e:
        # Unify different exception types to ValueError
        raise ValueError(e)


cdef Struct _fbthrift_struct_update_nested_field(Struct obj, list path_and_vals):
    field_to_nested_path_and_vals = defaultdict(list)
    cdef dict field_to_vals = {}
    for path, val in path_and_vals:
        field = path[0]
        if len(path) > 1:
            field_to_nested_path_and_vals[field].append((path[1:], val))
        else:
            field_to_vals[field] = val

    cdef dict updatedict = {}
    for field, val in field_to_vals.items():
        if field in field_to_nested_path_and_vals:
            conflicts = [
                ".".join([field] + p)
                for p, _ in field_to_nested_path_and_vals[field]
            ] + [field]
            raise ValueError("Conflicting overrides: {}".format(conflicts))
        updatedict[field] = val

    for field, nested_path_and_vals in field_to_nested_path_and_vals.items():
        updatedict[field] = _fbthrift_struct_update_nested_field(
            getattr(obj, field),
            nested_path_and_vals)

    return obj(**updatedict)
