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
    cThriftMetadata,
    cThriftStruct,
    cThriftException,
    cThriftService,
    cThriftEnum,
    ThriftStructType,
    ThriftUnionType,
    ThriftEnumType,
)
from libcpp.memory cimport make_shared

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
        return meta.structs[(<Struct?>obj_or_cls).__get_thrift_name__()]
    elif isinstance(obj_or_cls, GeneratedError):
        meta = ThriftMetadata.create(make_shared[cThriftMetadata]((<GeneratedError?>obj_or_cls).__get_metadata__()))
        return meta.exceptions[(<GeneratedError?>obj_or_cls).__get_thrift_name__()]
    elif isinstance(obj_or_cls, ServiceInterface):
        meta = ThriftMetadata.create(make_shared[cThriftMetadata]((<ServiceInterface?>obj_or_cls).__get_metadata__()))
        return meta.services[(<ServiceInterface?>obj_or_cls).__get_thrift_name__()]
    elif isinstance(obj_or_cls, Client):
        meta = ThriftMetadata.create(make_shared[cThriftMetadata]((<Client?>obj_or_cls).__get_metadata__()))
        return meta.services[(<Client?>obj_or_cls).__get_thrift_name__()]
    elif isinstance(obj_or_cls, CompiledEnum):
        meta = ThriftMetadata.create(make_shared[cThriftMetadata]((<CompiledEnum?>obj_or_cls).__get_metadata__()))
        return meta.enums[(<CompiledEnum?>obj_or_cls).__get_thrift_name__()]
    else:
        raise TypeError('No metadata information found')
