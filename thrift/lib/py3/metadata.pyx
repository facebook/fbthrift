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

cimport cython

import threading
from functools import wraps
from thrift.py3.types cimport CompiledEnum, Struct
from thrift.py3.exceptions cimport GeneratedError
from thrift.py3.server cimport ServiceInterface
from thrift.py3.client cimport Client
from apache.thrift.metadata.types cimport (
    ThriftMetadata,
    ThriftStruct,
    ThriftException,
    ThriftService,
    ThriftEnum,
    ThriftType,
    ThriftField,
    cThriftMetadata,
    cThriftStruct,
    cThriftException,
    cThriftService,
    cThriftEnum,
    ThriftStructType,
    ThriftUnionType,
    ThriftEnumType,
    ThriftPrimitiveType,
    ThriftListType,
    ThriftSetType,
    ThriftMapType,
    ThriftFunction,
    ThriftTypedefType,
    ThriftSinkType,
    ThriftStreamType,
)
from libcpp.memory cimport make_shared

cpdef enum ThriftKind:
    PRIMITIVE = 0
    LIST = 1
    SET = 2
    MAP = 3
    ENUM = 4
    STRUCT = 5
    UNION = 6
    TYPEDEF = 7
    STREAM = 8
    SINK = 9

cdef class ThriftTypeProxy:
    # This is actually a union of a bunch of thrift metadata types
    # whose closest shared parent is Struct, so this is just to make
    # the compiler happy
    cdef public Struct thriftType
    cdef public ThriftMetadata thriftMeta
    cdef public ThriftKind kind
    def __init__(self, thriftType, thriftMeta):
        self.thriftType = thriftType
        self.thriftMeta = thriftMeta

    @staticmethod
    def create(thriftType, thriftMeta):
        # Determine value and kind
        if thriftType.type == ThriftType.Type.t_list:
            return ThriftListProxy(thriftType.value, thriftMeta)
        elif thriftType.type == ThriftType.Type.t_set:
            return ThriftSetProxy(thriftType.value, thriftMeta)
        elif thriftType.type == ThriftType.Type.t_map:
            return ThriftMapProxy(thriftType.value, thriftMeta)
        elif thriftType.type == ThriftType.Type.t_enum:
            specialType = ThriftTypeProxy(thriftMeta.enums[thriftType.value.name], thriftMeta)
            specialType.kind = ThriftKind.ENUM
            return specialType
        elif thriftType.type == ThriftType.Type.t_struct:
            return ThriftStructProxy(thriftType.value.name, thriftMeta)
        elif thriftType.type == ThriftType.Type.t_union:
            return ThriftStructProxy(thriftType.value.name, thriftMeta)
        elif thriftType.type == ThriftType.Type.t_typedef:
            return ThriftTypedefProxy(thriftType.value, thriftMeta)
        elif thriftType.type == ThriftType.Type.t_stream:
            return ThriftStreamProxy(thriftType.value, thriftMeta)
        elif thriftType.type == ThriftType.Type.t_sink:
            return ThriftSinkProxy(thriftType.value, thriftMeta)
        specialType = ThriftTypeProxy(thriftType, thriftMeta)
        specialType.kind = ThriftKind.PRIMITIVE
        return specialType

    def as_primitive(self):
        if self.kind == ThriftKind.PRIMITIVE:
            return self.thriftType
        raise TypeError('Type is not primitive')

    def as_struct(self):
        if self.kind == ThriftKind.STRUCT or self.kind == ThriftKind.UNION:
            return self
        raise TypeError('Type is not a struct')

    def as_union(self):
        if self.kind == ThriftKind.UNION:
            return self
        raise TypeError('Type is not a union')

    def as_enum(self):
        if self.kind == ThriftKind.ENUM:
            return self.thriftType
        raise TypeError('Type is not an enum')

    def as_list(self):
        if self.kind == ThriftKind.LIST:
            return self
        raise TypeError('Type is not a list')

    def as_set(self):
        if self.kind == ThriftKind.SET:
            return self
        raise TypeError('Type is not a set')

    def as_typedef(self):
        if self.kind == ThriftKind.TYPEDEF:
            return self
        raise TypeError('Type is not a typedef')

    def as_stream(self):
        if self.kind == ThriftKind.STREAM:
            return self
        raise TypeError('Type is not a stream')

    def as_sink(self):
        if self.kind == ThriftKind.SINK:
            return self
        raise TypeError('Type is not a sink')

cdef class ThriftSetProxy(ThriftTypeProxy):
    cdef public ThriftTypeProxy valueType
    def __init__(self, thriftType, thriftMeta):
        super().__init__(thriftType, thriftMeta)
        self.kind = ThriftKind.SET
        self.valueType = ThriftTypeProxy.create(self.thriftType.valueType, self.thriftMeta)

cdef class ThriftListProxy(ThriftTypeProxy):
    cdef public ThriftTypeProxy valueType
    def __init__(self, thriftType, thriftMeta):
        super().__init__(thriftType, thriftMeta)
        self.kind = ThriftKind.LIST
        self.valueType = ThriftTypeProxy.create(self.thriftType.valueType, self.thriftMeta)

cdef class ThriftMapProxy(ThriftTypeProxy):
    cdef public ThriftTypeProxy valueType
    cdef public ThriftTypeProxy keyType
    def __init__(self, thriftType, thriftMeta):
        super().__init__(thriftType, thriftMeta)
        self.kind = ThriftKind.MAP
        self.valueType = ThriftTypeProxy.create(self.thriftType.valueType, self.thriftMeta)
        self.keyType = ThriftTypeProxy.create(self.thriftType.keyType, self.thriftMeta)

cdef class ThriftTypedefProxy(ThriftTypeProxy):
    cdef public ThriftTypeProxy underlyingType
    cdef public str name
    def __init__(self, thriftType, thriftMeta):
        super().__init__(thriftType, thriftMeta)
        self.kind = ThriftKind.TYPEDEF
        self.name = self.thriftType.name
        self.underlyingType = ThriftTypeProxy.create(self.thriftType.underlyingType, self.thriftMeta)

cdef class ThriftSinkProxy(ThriftTypeProxy):
    cdef public ThriftTypeProxy elemType
    cdef public ThriftTypeProxy initialResponseType
    def __init__(self, thriftType, thriftMeta):
        super().__init__(thriftType, thriftMeta)
        self.kind = ThriftKind.SINK
        self.elemType = ThriftTypeProxy.create(self.thriftType.elemType, self.thriftMeta)
        self.initialResponseType = ThriftTypeProxy.create(self.thriftType.initialResponseType, self.thriftMeta)

cdef class ThriftStreamProxy(ThriftTypeProxy):
    cdef public ThriftTypeProxy elemType
    cdef public ThriftTypeProxy finalResponseType
    cdef public ThriftTypeProxy initialResponseType
    def __init__(self, thriftType, thriftMeta):
        super().__init__(thriftType, thriftMeta)
        self.kind = ThriftKind.STREAM
        self.elemType = ThriftTypeProxy.create(self.thriftType.elemType, self.thriftMeta)
        self.finalResponseType = ThriftTypeProxy.create(self.thriftType.finalResponseType, self.thriftMeta)
        self.initialResponseType = ThriftTypeProxy.create(self.thriftType.initialResponseType, self.thriftMeta)

cdef class ThriftFieldProxy:
    cdef public ThriftTypeProxy type
    cdef public ThriftField thriftType
    cdef public ThriftMetadata thriftMeta
    cdef public int id
    cdef public str name
    cdef public int is_optional
    def __init__(self, thriftType, thriftMeta):
        self.type = ThriftTypeProxy.create(thriftType.type, thriftMeta)
        self.thriftType = thriftType
        self.thriftMeta = thriftMeta
        self.id = self.thriftType.id
        self.name = self.thriftType.name
        self.is_optional = self.thriftType.is_optional

cdef class ThriftStructProxy(ThriftTypeProxy):
    cdef public str name
    cdef public int is_union
    def __init__(self, name, thriftMeta):
        super().__init__(thriftMeta.structs[name], thriftMeta)
        self.name = self.thriftType.name
        self.is_union = self.thriftType.is_union

        if self.is_union:
            self.kind = ThriftKind.UNION
        else:
            self.kind = ThriftKind.STRUCT
    @property
    def fields(self):
        for field in self.thriftType.fields:
            yield ThriftFieldProxy(field, self.thriftMeta)

cdef class ThriftExceptionProxy:
    cdef public ThriftException thriftType
    cdef public ThriftMetadata thriftMeta
    cdef public str name
    def __init__(self, name, thriftMeta):
        self.thriftType = thriftMeta.exceptions[name]
        self.thriftMeta = thriftMeta
        self.name = self.thriftType.name
    @property
    def fields(self):
        for field in self.thriftType.fields:
            yield ThriftFieldProxy(field, self.thriftMeta)

cdef class ThriftFunctionProxy:
    cdef public str name
    cdef public ThriftFunction thriftType
    cdef public ThriftMetadata thriftMeta
    cdef public ThriftTypeProxy return_type
    cdef public int is_oneway
    def __init__(self, thriftType, thriftMeta):
        self.name = thriftType.name
        self.thriftType = thriftType
        self.thriftMeta = thriftMeta
        self.return_type = ThriftTypeProxy.create(self.thriftType.return_type, self.thriftMeta)
        self.is_oneway = self.thriftType.is_oneway
    @property
    def arguments(self):
        for argument in self.thriftType.arguments:
            yield ThriftFieldProxy(argument, self.thriftMeta)
    @property
    def exceptions(self):
        for exception in self.thriftType.exceptions:
            yield ThriftFieldProxy(exception, self.thriftMeta)

cdef class ThriftServiceProxy:
    cdef public ThriftService thriftType
    cdef public str name
    cdef public ThriftMetadata thriftMeta
    cdef public ThriftServiceProxy parent
    def __init__(self, name, thriftMeta):
        self.thriftType = thriftMeta.services[name]
        self.name = self.thriftType.name
        self.thriftMeta = thriftMeta
        self.parent = None if self.thriftType.parent is None else ThriftServiceProxy(
            self.thriftMeta.services[self.thriftType.parent],
            self.thriftMeta
        )
    @property
    def functions(self):
        for function in self.thriftType.functions:
            yield ThriftFunctionProxy(function, self.thriftMeta)

def gen_metadata(obj_or_cls):
    if hasattr(obj_or_cls, "getThriftModuleMetadata"):
        meta = obj_or_cls.getThriftModuleMetadata()
        return meta
    if isinstance(obj_or_cls, type):
    # Enums need to be special cased here because you cannot init an enum with CompiledEnum()
        if issubclass(obj_or_cls, CompiledEnum):
            obj_or_cls = obj_or_cls(0)
        else:
            obj_or_cls = obj_or_cls()
    if isinstance(obj_or_cls, Struct):
        meta = ThriftMetadata.create(make_shared[cThriftMetadata]((<Struct?>obj_or_cls).__get_metadata__()))
        return ThriftStructProxy((<Struct?>obj_or_cls).__get_thrift_name__(), meta)
    elif isinstance(obj_or_cls, GeneratedError):
        meta = ThriftMetadata.create(make_shared[cThriftMetadata]((<GeneratedError?>obj_or_cls).__get_metadata__()))
        return ThriftExceptionProxy((<GeneratedError?>obj_or_cls).__get_thrift_name__(), meta)
    elif isinstance(obj_or_cls, ServiceInterface):
        meta = ThriftMetadata.create(make_shared[cThriftMetadata]((<ServiceInterface?>obj_or_cls).__get_metadata__()))
        return ThriftServiceProxy((<ServiceInterface?>obj_or_cls).__get_thrift_name__(), meta)
    elif isinstance(obj_or_cls, Client):
        meta = ThriftMetadata.create(make_shared[cThriftMetadata]((<Client?>obj_or_cls).__get_metadata__()))
        return ThriftServiceProxy((<Client?>obj_or_cls).__get_thrift_name__(), meta)
    elif isinstance(obj_or_cls, CompiledEnum):
        meta = ThriftMetadata.create(make_shared[cThriftMetadata]((<CompiledEnum?>obj_or_cls).__get_metadata__()))
        return meta.enums[(<CompiledEnum?>obj_or_cls).__get_thrift_name__()]
    else:
        raise TypeError('No metadata information found')
