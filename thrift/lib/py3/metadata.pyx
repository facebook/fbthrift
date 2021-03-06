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

from libcpp.utility cimport move
from thrift.py3.types cimport CompiledEnum, Struct
from thrift.py3.exceptions cimport GeneratedError
from thrift.py3.server cimport ServiceInterface
from thrift.py3.client cimport Client
from thrift.py3.common cimport MetadataBox
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
    # A union of a bunch of thrift metadata types
    cdef readonly object thriftType
    cdef readonly ThriftMetadata thriftMeta
    cdef readonly ThriftKind kind

    def __init__(self, object thriftType not None, ThriftMetadata thriftMeta not None):
        if not isinstance(thriftType, (
            ThriftStruct,
            ThriftEnum,
            ThriftPrimitiveType,
            ThriftListType,
            ThriftSetType,
            ThriftMapType,
            ThriftTypedefType,
            ThriftSinkType,
            ThriftStreamType,
        )):
            raise TypeError(f"{thriftType!r} is not a known thrift type.")
        self.thriftType = thriftType
        self.thriftMeta = thriftMeta

    @staticmethod
    cdef create(ThriftType thriftType, ThriftMetadata thriftMeta):
        # Determine value and kind
        if thriftType.type is ThriftType.Type.t_list:
            return ThriftListProxy(thriftType.value, thriftMeta)
        elif thriftType.type is ThriftType.Type.t_set:
            return ThriftSetProxy(thriftType.value, thriftMeta)
        elif thriftType.type is ThriftType.Type.t_map:
            return ThriftMapProxy(thriftType.value, thriftMeta)
        elif thriftType.type is ThriftType.Type.t_enum:
            specialType = ThriftTypeProxy(thriftMeta.enums[thriftType.value.name], thriftMeta)
            specialType.kind = ThriftKind.ENUM
            return specialType
        elif thriftType.type is ThriftType.Type.t_struct:
            return ThriftStructProxy(thriftType.value.name, thriftMeta)
        elif thriftType.type is ThriftType.Type.t_union:
            return ThriftStructProxy(thriftType.value.name, thriftMeta)
        elif thriftType.type is ThriftType.Type.t_typedef:
            return ThriftTypedefProxy(thriftType.value, thriftMeta)
        elif thriftType.type is ThriftType.Type.t_stream:
            return ThriftStreamProxy(thriftType.value, thriftMeta)
        elif thriftType.type is ThriftType.Type.t_sink:
            return ThriftSinkProxy(thriftType.value, thriftMeta)
        specialType = ThriftTypeProxy(thriftType.value, thriftMeta)
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

    def as_map(self):
        if self.kind == ThriftKind.MAP:
            return self
        raise TypeError('Type is not a map')

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
    cdef readonly ThriftTypeProxy valueType

    def __init__(self, ThriftSetType thriftType not None, ThriftMetadata thriftMeta not None):
        super().__init__(thriftType, thriftMeta)
        self.kind = ThriftKind.SET
        self.valueType = ThriftTypeProxy.create(self.thriftType.valueType, self.thriftMeta)


cdef class ThriftListProxy(ThriftTypeProxy):
    cdef readonly ThriftTypeProxy valueType

    def __init__(self, ThriftListType thriftType not None, ThriftMetadata thriftMeta not None):
        super().__init__(thriftType, thriftMeta)
        self.kind = ThriftKind.LIST
        self.valueType = ThriftTypeProxy.create(self.thriftType.valueType, self.thriftMeta)


cdef class ThriftMapProxy(ThriftTypeProxy):
    cdef readonly ThriftTypeProxy valueType
    cdef readonly ThriftTypeProxy keyType

    def __init__(self, ThriftMapType thriftType not None, ThriftMetadata thriftMeta not None):
        super().__init__(thriftType, thriftMeta)
        self.kind = ThriftKind.MAP
        self.valueType = ThriftTypeProxy.create(self.thriftType.valueType, self.thriftMeta)
        self.keyType = ThriftTypeProxy.create(self.thriftType.keyType, self.thriftMeta)


cdef class ThriftTypedefProxy(ThriftTypeProxy):
    cdef readonly ThriftTypeProxy underlyingType
    cdef readonly str name

    def __init__(self, ThriftTypedefType thriftType not None, ThriftMetadata thriftMeta not None):
        super().__init__(thriftType, thriftMeta)
        self.kind = ThriftKind.TYPEDEF
        self.name = self.thriftType.name
        self.underlyingType = ThriftTypeProxy.create(self.thriftType.underlyingType, self.thriftMeta)


cdef class ThriftSinkProxy(ThriftTypeProxy):
    cdef readonly ThriftTypeProxy elemType
    cdef readonly ThriftTypeProxy initialResponseType

    def __init__(self, ThriftSinkType thriftType not None, ThriftMetadata thriftMeta not None):
        super().__init__(thriftType, thriftMeta)
        self.kind = ThriftKind.SINK
        self.elemType = ThriftTypeProxy.create(self.thriftType.elemType, self.thriftMeta)
        self.initialResponseType = ThriftTypeProxy.create(self.thriftType.initialResponseType, self.thriftMeta)


cdef class ThriftStreamProxy(ThriftTypeProxy):
    cdef readonly ThriftTypeProxy elemType
    cdef readonly ThriftTypeProxy finalResponseType
    cdef readonly ThriftTypeProxy initialResponseType

    def __init__(self, ThriftStreamType thriftType not None, ThriftMetadata thriftMeta not None):
        super().__init__(thriftType, thriftMeta)
        self.kind = ThriftKind.STREAM
        self.elemType = ThriftTypeProxy.create(self.thriftType.elemType, self.thriftMeta)
        self.finalResponseType = ThriftTypeProxy.create(self.thriftType.finalResponseType, self.thriftMeta)
        self.initialResponseType = ThriftTypeProxy.create(self.thriftType.initialResponseType, self.thriftMeta)


cdef class ThriftFieldProxy:
    cdef readonly ThriftTypeProxy type
    cdef readonly ThriftField thriftType
    cdef readonly ThriftMetadata thriftMeta
    cdef readonly int id
    cdef readonly str name
    cdef readonly int is_optional

    def __init__(self, ThriftField thriftType not None, ThriftMetadata thriftMeta not None):
        self.type = ThriftTypeProxy.create(thriftType.type, thriftMeta)
        self.thriftType = thriftType
        self.thriftMeta = thriftMeta
        self.id = self.thriftType.id
        self.name = self.thriftType.name
        self.is_optional = self.thriftType.is_optional


cdef class ThriftStructProxy(ThriftTypeProxy):
    cdef readonly str name
    cdef readonly int is_union

    def __init__(self, str name not None, ThriftMetadata thriftMeta not None):
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
    cdef readonly ThriftException thriftType
    cdef readonly ThriftMetadata thriftMeta
    cdef readonly str name

    def __init__(self, str name not None, ThriftMetadata thriftMeta not None):
        self.thriftType = thriftMeta.exceptions[name]
        self.thriftMeta = thriftMeta
        self.name = self.thriftType.name

    @property
    def fields(self):
        for field in self.thriftType.fields:
            yield ThriftFieldProxy(field, self.thriftMeta)


cdef class ThriftFunctionProxy:
    cdef readonly str name
    cdef readonly ThriftFunction thriftType
    cdef readonly ThriftMetadata thriftMeta
    cdef readonly ThriftTypeProxy return_type
    cdef readonly int is_oneway

    def __init__(self, ThriftFunction thriftType not None, ThriftMetadata thriftMeta not None):
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
    cdef readonly ThriftService thriftType
    cdef readonly str name
    cdef readonly ThriftMetadata thriftMeta
    cdef readonly ThriftServiceProxy parent

    def __init__(self, str name not None, ThriftMetadata thriftMeta not None):
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
        return obj_or_cls.getThriftModuleMetadata()

    cls = obj_or_cls if isinstance(obj_or_cls, type) else type(obj_or_cls)

    if not issubclass(cls, (Struct, GeneratedError, ServiceInterface, Client, CompiledEnum)):
        raise TypeError(f'{cls!r} is not a thrift-py3 type.')

    # get the box
    cdef MetadataBox box = cls.__get_metadata__()
    # unbox the box
    cdef ThriftMetadata meta = ThriftMetadata.create(move(box._cpp_obj))
    cdef str name = cls.__get_thrift_name__()

    if issubclass(cls, Struct):
        return ThriftStructProxy(name, meta)
    elif issubclass(cls, GeneratedError):
        return ThriftExceptionProxy(name, meta)
    elif issubclass(cls, ServiceInterface):
        return ThriftServiceProxy(name, meta)
    elif issubclass(cls, Client):
        return ThriftServiceProxy(name, meta)
    elif issubclass(cls, CompiledEnum):
        return meta.enums[name]
    else:
        raise TypeError(f'unsupported thrift-py3 type: {cls!r}.')
