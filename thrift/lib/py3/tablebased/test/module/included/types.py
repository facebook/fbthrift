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


# this is the generated pure Python code from Thrift IDL

import enum

import thrift.py3lite.types as _fbthrift_py3lite_types


class MyEnum(enum.Enum):
    ONE = 1
    TWO = 2


class IncludedStruct(metaclass=_fbthrift_py3lite_types.StructMeta):
    # # spec for tabled-based serializer
    _fbthrift_SPEC = (
        (
            1,  # id
            True,  # isUnqualified
            "intField",  # name
            _fbthrift_py3lite_types.typeinfo_i32,  # typeinfo
            None,  # default value
        ),
        (
            2,  # id
            True,  # isUnqualified
            "listOfIntField",  # name
            _fbthrift_py3lite_types.ListTypeInfo(
                _fbthrift_py3lite_types.typeinfo_i32
            ),  # typeinfo
            None,  # default value
        ),
    )


_fbthrift_py3lite_types.fill_specs(IncludedStruct)
