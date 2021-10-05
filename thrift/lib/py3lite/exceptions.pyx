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

import copy
import functools

from thrift.py3lite.types cimport StructInfo, createStructTuple, set_struct_field


cdef make_fget_error(i):
    return functools.cached_property(lambda self: (<GeneratedError>self)._fbthrift_get_field_value(i))


class GeneratedErrorMeta(type):
    def __new__(cls, name, bases, dct):
        fields = dct.pop('_fbthrift_SPEC')
        num_fields = len(fields)
        dct["_fbthrift_struct_info"] = StructInfo(name, fields)
        for i, f in enumerate(fields):
            dct[f[2]] = make_fget_error(i)
        return super().__new__(cls, name, (GeneratedError,), dct)

    def _fbthrift_fill_spec(cls):
        (<StructInfo>cls._fbthrift_struct_info).fill()

    def _fbthrift_store_field_values(cls):
        (<StructInfo>cls._fbthrift_struct_info).store_field_values()


cdef class GeneratedError(Error):
    def __cinit__(self):
        cdef StructInfo info = self._fbthrift_struct_info
        self._fbthrift_data = createStructTuple(
            info.cpp_obj.get().getStructInfo()
        )

    def __init__(self, *args, **kwargs):
        cdef StructInfo info = self._fbthrift_struct_info
        names_iter = iter(info.name_to_index)
        for idx, value in enumerate(args):
            try:
                name = next(names_iter)
            except StopIteration:
                raise TypeError(f"{type(self).__name__}() only takes {idx} arguments")
            if name in kwargs:
                raise TypeError(f"__init__() got multiple values for argument '{name}'")
            kwargs[name] = value

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
        # for builtin.BaseException
        super().__init__(*(value for _, value in self))

    def __iter__(self):
        cdef StructInfo info = self._fbthrift_struct_info
        for name in info.name_to_index:
            yield name, getattr(self, name)

    cdef _fbthrift_get_field_value(self, int16_t index):
        data = self._fbthrift_data[index + 1]
        if data is None:
            return
        cdef StructInfo info = self._fbthrift_struct_info
        return info.type_infos[index].to_python_value(data)

    def __copy__(GeneratedError self):
        # deep copy the instance
        return self._fbthrift_create(copy.deepcopy(self._fbthrift_data))

    def __lt__(self, other):
        if type(self) != type(other):
            return NotImplemented
        ret = (<GeneratedError>self)._fbthrift_data[1:] < (<GeneratedError>other)._fbthrift_data[1:]
        return ret

    def __le__(self, other):
        return self < other or self == other

    def __eq__(GeneratedError self, other):
        if type(other) != type(self):
            return False
        cdef StructInfo info = self._fbthrift_struct_info
        for name in info.name_to_index:
            if getattr(self, name) != getattr(other, name):
                return False
        return True

    def __hash__(GeneratedError self):
        return hash(self._fbthrift_data)

    @classmethod
    def _fbthrift_create(cls, data):
        cdef GeneratedError inst = cls.__new__(cls)
        inst._fbthrift_data = data
        return inst
