#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
from collections import namedtuple, OrderedDict

from thrift.Thrift import TType


__all__ = ['Serializer', 'struct_to_dict', 'parse_struct_spec']
StructField = namedtuple('StructField',
                         'id type name type_args default req_type')


def parse_struct_spec(struct):
    """
    Given a thrift struct return a generator of parsed field information

    StructField fields:
        id - the field number
        type - a Thrift.TType
        name - the field name
        type_args - type arguments (ex: Key type Value type for maps)
        default - the default value
        req_type - the field required setting
            (0: Required, 1: Optional, 2: Optional IN, Required OUT)

    :param struct: a thrift struct
    :return: a generator of StructField tuples
    """
    for field in struct.thrift_spec:
        if not field:
            continue
        yield StructField._make(field)


def struct_to_dict(struct):
    """
    Given a Thrift Struct convert it into a dict
    :param struct: a thrift struct
    :return: OrderedDict
    """
    adict = OrderedDict()
    union = struct.isUnion()
    if union and struct.field == 0:
        # if struct.field is 0 then it is unset escape
        return adict
    for field in parse_struct_spec(struct):
        if union:
            if field.id == struct.field:
                value = struct.value
            else:
                continue
        else:
            value = getattr(struct, field.name, field.default)
        if value != field.default:
            if field.type == TType.STRUCT:
                sub_dict = struct_to_dict(value)
                if sub_dict:  # Do not include empty sub structs
                    adict[field.name] = sub_dict
            else:
                adict[field.name] = value
        if union:  # If we got this far then we have the union value
            break
    return adict
