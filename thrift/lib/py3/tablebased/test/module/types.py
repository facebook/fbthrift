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

import thrift.py3lite.exceptions as _fbthrift_py3lite_exceptions
import thrift.py3lite.test.included.types as _thrift_py3lite_test_included_types
import thrift.py3lite.types as _fbthrift_py3lite_types


class MyStruct(metaclass=_fbthrift_py3lite_types.StructMeta):
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
        (
            3,  # id
            False,  # isUnqualified
            "optionalField",  # name
            _fbthrift_py3lite_types.ListTypeInfo(
                _fbthrift_py3lite_types.typeinfo_i32
            ),  # typeinfo
            None,  # default value
        ),
        (
            4,  # id
            True,  # isUnqualified
            "enumField",  # name
            _fbthrift_py3lite_types.EnumTypeInfo(
                _thrift_py3lite_test_included_types.MyEnum
            ),  # typeinfo
            None,  # default value
        ),
    )


class RecursiveStruct(metaclass=_fbthrift_py3lite_types.StructMeta):
    _fbthrift_SPEC = (
        (
            1,  # id
            False,  # isUnqualified
            "recursiveField",  # name
            lambda: _fbthrift_py3lite_types.StructTypeInfo(RecursiveStruct),  # typeinfo
            None,  # default value
        ),
        (
            2,  # id
            True,  # isUnqualified
            "AnotherStructField",  # name
            lambda: _fbthrift_py3lite_types.StructTypeInfo(AnotherStruct),  # typeinfo
            None,  # default value
        ),
    )


class AnotherStruct(metaclass=_fbthrift_py3lite_types.StructMeta):
    # spec for tabled-based serializer
    _fbthrift_SPEC = (
        (
            1,  # id
            True,  # isUnqualified
            "structField",  # name
            _fbthrift_py3lite_types.StructTypeInfo(
                _thrift_py3lite_test_included_types.IncludedStruct
            ),  # typeinfo
            None,  # default value
        ),
        (
            2,  # id
            True,  # isUnqualified
            "listOfStructField",  # name
            _fbthrift_py3lite_types.ListTypeInfo(
                _fbthrift_py3lite_types.StructTypeInfo(MyStruct)
            ),  # typeinfo
            None,  # default value
        ),
        (
            3,  # id
            True,  # isUnqualified
            "listOflistOfStructField",  # name
            _fbthrift_py3lite_types.ListTypeInfo(
                _fbthrift_py3lite_types.ListTypeInfo(
                    _fbthrift_py3lite_types.StructTypeInfo(MyStruct)
                )
            ),
            None,  # default value
        ),
    )


class MyUnion(metaclass=_fbthrift_py3lite_types.UnionMeta):
    _fbthrift_SPEC = (
        (
            2,  # id
            True,  # isUnqualified
            "intField",  # name
            _fbthrift_py3lite_types.typeinfo_i32,  # typeinfo
            None,  # default value
        ),
        (
            3,  # id
            True,  # isUnqualified
            "MyStructField",  # name
            _fbthrift_py3lite_types.StructTypeInfo(MyStruct),  # typeinfo
            None,  # default value
        ),
        (
            4,  # id
            True,  # isUnqualified
            "AnotherStructField",  # name
            _fbthrift_py3lite_types.StructTypeInfo(AnotherStruct),  # typeinfo
            None,  # default value
        ),
    )


class StructWithAUnion(metaclass=_fbthrift_py3lite_types.StructMeta):
    # spec for tabled-based serializer
    _fbthrift_SPEC = (
        (
            1,  # id
            True,  # isUnqualified
            "unionField",  # name
            lambda: _fbthrift_py3lite_types.StructTypeInfo(MyUnion),  # typeinfo
            None,  # default value
        ),
    )


class PrimitiveStruct(metaclass=_fbthrift_py3lite_types.StructMeta):
    # spec for tabled-based serializer
    _fbthrift_SPEC = (
        (
            1,  # id
            True,  # isUnqualified
            "boolField",  # name
            _fbthrift_py3lite_types.typeinfo_bool,  # typeinfo
            None,  # default value
        ),
        (
            2,  # id
            True,  # isUnqualified
            "byteField",  # name
            _fbthrift_py3lite_types.typeinfo_byte,  # typeinfo
            None,  # default value
        ),
        (
            3,  # id
            True,  # isUnqualified
            "i16Field",  # name
            _fbthrift_py3lite_types.typeinfo_i16,  # typeinfo
            None,  # default value
        ),
        (
            4,  # id
            True,  # isUnqualified
            "i32Field",  # name
            _fbthrift_py3lite_types.typeinfo_i32,  # typeinfo
            None,  # default value
        ),
        (
            5,  # id
            True,  # isUnqualified
            "i64Field",  # name
            _fbthrift_py3lite_types.typeinfo_i64,  # typeinfo
            None,  # default value
        ),
        (
            6,  # id
            True,  # isUnqualified
            "doubleField",  # name
            _fbthrift_py3lite_types.typeinfo_double,  # typeinfo
            None,  # default value
        ),
        (
            7,  # id
            True,  # isUnqualified
            "floatField",  # name
            _fbthrift_py3lite_types.typeinfo_float,  # typeinfo
            None,  # default value
        ),
    )


class StringStruct(metaclass=_fbthrift_py3lite_types.StructMeta):
    # spec for tabled-based serializer
    _fbthrift_SPEC = (
        (
            1,  # id
            True,  # isUnqualified
            "stringField",  # name
            _fbthrift_py3lite_types.typeinfo_string,  # typeinfo
            None,  # default value
        ),
        (
            2,  # id
            True,  # isUnqualified
            "binaryField",  # name
            _fbthrift_py3lite_types.typeinfo_binary,  # typeinfo
            None,  # default value
        ),
    )


class SetStruct(metaclass=_fbthrift_py3lite_types.StructMeta):
    # spec for tabled-based serializer
    _fbthrift_SPEC = (
        (
            1,  # id
            True,  # isUnqualified
            "setOfI32Field",  # name
            _fbthrift_py3lite_types.SetTypeInfo(
                _fbthrift_py3lite_types.typeinfo_i32
            ),  # typeinfo
            None,  # default value
        ),
        (
            2,  # id
            True,  # isUnqualified
            "setOfStructField",  # name
            _fbthrift_py3lite_types.SetTypeInfo(
                _fbthrift_py3lite_types.StructTypeInfo(MyStruct)
            ),  # typeinfo
            None,  # default value
        ),
        (
            3,  # id
            True,  # isUnqualified
            "setOfEnumField",  # name
            _fbthrift_py3lite_types.SetTypeInfo(
                _fbthrift_py3lite_types.EnumTypeInfo(
                    _thrift_py3lite_test_included_types.MyEnum
                )
            ),  # typeinfo
            None,  # default value
        ),
        (
            4,  # id
            True,  # isUnqualified
            "setOfStringField",  # name
            _fbthrift_py3lite_types.SetTypeInfo(
                _fbthrift_py3lite_types.typeinfo_string
            ),  # typeinfo
            None,  # default value
        ),
    )


class MapStruct(metaclass=_fbthrift_py3lite_types.StructMeta):
    # spec for tabled-based serializer
    _fbthrift_SPEC = (
        (
            1,  # id
            True,  # isUnqualified
            "mapOfStrI32Field",  # name
            _fbthrift_py3lite_types.MapTypeInfo(
                _fbthrift_py3lite_types.typeinfo_string,
                _fbthrift_py3lite_types.typeinfo_i32,
            ),  # typeinfo
            None,  # default value
        ),
        (
            2,  # id
            True,  # isUnqualified
            "mapOfStrStructField",  # name
            _fbthrift_py3lite_types.MapTypeInfo(
                _fbthrift_py3lite_types.typeinfo_string,
                _fbthrift_py3lite_types.StructTypeInfo(MyStruct),
            ),  # typeinfo
            None,  # default value
        ),
        (
            3,  # id
            True,  # isUnqualified
            "mapOfStrEnumField",  # name
            _fbthrift_py3lite_types.MapTypeInfo(
                _fbthrift_py3lite_types.typeinfo_string,
                _fbthrift_py3lite_types.EnumTypeInfo(
                    _thrift_py3lite_test_included_types.MyEnum
                ),
            ),  # typeinfo
            None,  # default value
        ),
    )


class MyException(metaclass=_fbthrift_py3lite_exceptions.GeneratedErrorMeta):
    # spec for tabled-based serializer
    _fbthrift_SPEC = (
        (
            1,  # id
            True,  # isUnqualified
            "error_message",  # name
            _fbthrift_py3lite_types.typeinfo_string,  # typeinfo
            None,  # default value
        ),
        (
            2,  # id
            True,  # isUnqualified
            "internal_error_message",  # name
            _fbthrift_py3lite_types.typeinfo_string,  # typeinfo
            None,  # default value
        ),
    )

    def __str__(self):
        return self.internal_error_message


class StructWithDefaults(metaclass=_fbthrift_py3lite_types.StructMeta):
    # # spec for tabled-based serializer
    _fbthrift_SPEC = (
        (
            1,  # id
            True,  # isUnqualified
            "intField",  # name
            _fbthrift_py3lite_types.typeinfo_i32,  # typeinfo
            42,  # default value
        ),
        (
            2,  # id
            True,  # isUnqualified
            "listOfIntField",  # name
            _fbthrift_py3lite_types.ListTypeInfo(
                _fbthrift_py3lite_types.typeinfo_i32
            ),  # typeinfo
            [12, 34],  # default value
        ),
        (
            3,  # id
            True,  # isUnqualified
            "simpleStructField",  # name
            lambda: _fbthrift_py3lite_types.StructTypeInfo(SimpleStruct),  # typeinfo
            lambda: SimpleStruct(intField=678),  # default value
        ),
        (
            4,  # id
            True,  # isUnqualified
            "enumField",  # name
            _fbthrift_py3lite_types.EnumTypeInfo(
                _thrift_py3lite_test_included_types.MyEnum
            ),  # typeinfo
            _thrift_py3lite_test_included_types.MyEnum.TWO,  # default value
        ),
        (
            5,  # id
            True,  # isUnqualified
            "setOfI32Field",  # name
            _fbthrift_py3lite_types.SetTypeInfo(
                _fbthrift_py3lite_types.typeinfo_i32
            ),  # typeinfo
            {24, 65, 99},  # default value
        ),
        (
            6,  # id
            True,  # isUnqualified
            "mapOfStrI32Field",  # name
            _fbthrift_py3lite_types.MapTypeInfo(
                _fbthrift_py3lite_types.typeinfo_string,
                _fbthrift_py3lite_types.typeinfo_i32,
            ),  # typeinfo
            {"a": 24, "b": 99},  # default value
        ),
        (
            7,  # id
            True,  # isUnqualified
            "stringField",  # name
            _fbthrift_py3lite_types.typeinfo_string,  # typeinfo
            "awesome string",  # default value
        ),
        (
            8,  # id
            True,  # isUnqualified
            "binaryField",  # name
            _fbthrift_py3lite_types.typeinfo_binary,  # typeinfo
            b"awesome bytes",  # default value
        ),
    )


class SimpleStruct(metaclass=_fbthrift_py3lite_types.StructMeta):
    # # spec for tabled-based serializer
    _fbthrift_SPEC = (
        (
            1,  # id
            True,  # isUnqualified
            "intField",  # name
            _fbthrift_py3lite_types.typeinfo_i32,  # typeinfo
            99,  # default value
        ),
    )


class EmptyStruct(metaclass=_fbthrift_py3lite_types.StructMeta):
    # # spec for tabled-based serializer
    _fbthrift_SPEC = ()


_fbthrift_py3lite_types.fill_specs(
    MyStruct,
    AnotherStruct,
    RecursiveStruct,
    MyUnion,
    StructWithAUnion,
    PrimitiveStruct,
    StringStruct,
    SetStruct,
    MapStruct,
    MyException,
    StructWithDefaults,
    SimpleStruct,
    EmptyStruct,
)
