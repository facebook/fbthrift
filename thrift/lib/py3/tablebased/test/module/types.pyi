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
import typing

import thrift.py3lite.exceptions as _fbthrift_py3lite_exceptions
import thrift.py3lite.test.included.types as _thrift_py3lite_test_included_types
import thrift.py3lite.types as _fbthrift_py3lite_types

class MyStruct(_fbthrift_py3lite_types.Struct):
    intField: int
    listOfIntField: typing.Sequence[int]
    optionalField: typing.Optional[typing.Sequence[int]]
    enumField: _thrift_py3lite_test_included_types.MyEnum
    def __init__(
        self,
        *,
        intField: int = ...,
        listOfIntField: typing.Sequence[int] = ...,
        optionalField: typing.Sequence[int] = ...,
        enumField: _thrift_py3lite_test_included_types.MyEnum = ...,
    ) -> None: ...
    def __call__(
        self,
        *,
        intField: typing.Optional[int] = ...,
        listOfIntField: typing.Optional[typing.Sequence[int]] = ...,
        optionalField: typing.Optional[typing.Sequence[int]] = ...,
        enumField: typing.Optional[_thrift_py3lite_test_included_types.MyEnum] = ...,
    ) -> MyStruct: ...
    def __iter__(
        self,
    ) -> typing.Iterator[
        typing.Tuple[
            str,
            typing.Union[
                int, typing.Sequence[int], _thrift_py3lite_test_included_types.MyEnum
            ],
        ]
    ]: ...

class RecursiveStruct(_fbthrift_py3lite_types.Struct):
    recursiveField: RecursiveStruct
    AnotherStructField: AnotherStruct
    def __init__(
        self,
        *,
        recursiveField: RecursiveStruct = ...,
        AnotherStructField: AnotherStruct = ...,
    ) -> None: ...
    def __call__(
        self,
        *,
        recursiveField: typing.Optional[RecursiveStruct] = ...,
        AnotherStructField: typing.Optional[AnotherStruct] = ...,
    ) -> RecursiveStruct: ...
    def __iter__(
        self,
    ) -> typing.Iterator[
        typing.Tuple[str, typing.Union[RecursiveStruct, AnotherStruct]]
    ]: ...

class AnotherStruct(_fbthrift_py3lite_types.Struct):
    structField: _thrift_py3lite_test_included_types.IncludedStruct
    listOfStructField: typing.Sequence[MyStruct]
    listOflistOfStructField: typing.Sequence[typing.Sequence[MyStruct]]
    def __init__(
        self,
        *,
        structField: _thrift_py3lite_test_included_types.IncludedStruct = ...,
        listOfStructField: typing.Sequence[MyStruct] = ...,
        listOflistOfStructField: typing.Sequence[typing.Sequence[MyStruct]] = ...,
    ) -> None: ...
    def __call__(
        self,
        *,
        structField: typing.Optional[
            _thrift_py3lite_test_included_types.IncludedStruct
        ] = ...,
        listOfStructField: typing.Optional[typing.Sequence[MyStruct]] = ...,
        listOflistOfStructField: typing.Optional[
            typing.Sequence[typing.Sequence[MyStruct]]
        ] = ...,
    ) -> AnotherStruct: ...
    def __iter__(
        self,
    ) -> typing.Iterator[
        typing.Tuple[
            str,
            typing.Union[
                _thrift_py3lite_test_included_types.IncludedStruct,
                typing.Sequence[MyStruct],
                typing.Sequence[typing.Sequence[MyStruct]],
            ],
        ]
    ]: ...

class MyUnion(_fbthrift_py3lite_types.Union):
    class Type(enum.Enum):
        EMPTY = 0
        intField = 2
        MyStructField = 3
        AnotherStructField = 4
    type: Type
    value: typing.Union[int, MyStruct, AnotherStruct]
    intField: int
    MyStructField: MyStruct
    AnotherStructField: AnotherStruct
    def __init__(
        self,
        *,
        intField: int = ...,
        MyStructField: MyStruct = ...,
        AnotherStructField: AnotherStruct = ...,
    ) -> None: ...
    def get_type(self) -> Type: ...

class StructWithAUnion(_fbthrift_py3lite_types.Struct):
    unionField: MyUnion
    def __init__(
        self,
        *,
        unionField: MyUnion = ...,
    ) -> None: ...
    def __call__(
        self,
        *,
        unionField: typing.Optional[MyUnion] = ...,
    ) -> StructWithAUnion: ...
    def __iter__(
        self,
    ) -> typing.Iterator[typing.Tuple[str, typing.Union[MyUnion],]]: ...

class PrimitiveStruct(_fbthrift_py3lite_types.Struct):
    boolField: bool
    byteField: int
    i16Field: int
    i32Field: int
    i64Field: int
    doubleField: float
    floatField: float
    def __init__(
        self,
        *,
        boolField: bool = ...,
        byteField: int = ...,
        i16Field: int = ...,
        i32Field: int = ...,
        i64Field: int = ...,
        doubleField: float = ...,
        floatField: float = ...,
    ) -> None: ...
    def __call__(
        self,
        *,
        boolField: typing.Optional[bool] = ...,
        byteField: typing.Optional[int] = ...,
        i16Field: typing.Optional[int] = ...,
        i32Field: typing.Optional[int] = ...,
        i64Field: typing.Optional[int] = ...,
        doubleField: typing.Optional[float] = ...,
        floatField: typing.Optional[float] = ...,
    ) -> PrimitiveStruct: ...
    def __iter__(
        self,
    ) -> typing.Iterator[typing.Tuple[str, typing.Union[bool, int, float]]]: ...

class StringStruct(_fbthrift_py3lite_types.Struct):
    stringField: str
    binaryField: bytes
    def __init__(
        self,
        *,
        stringField: str = ...,
        binaryField: bytes = ...,
    ) -> None: ...
    def __call__(
        self,
        *,
        stringField: typing.Optional[str] = ...,
        binaryField: typing.Optional[bytes] = ...,
    ) -> StringStruct: ...
    def __iter__(
        self,
    ) -> typing.Iterator[typing.Tuple[str, typing.Union[str, bytes]]]: ...

class SetStruct(_fbthrift_py3lite_types.Struct):
    setOfI32Field: typing.AbstractSet[int]
    setOfStructField: typing.AbstractSet[MyStruct]
    setOfEnumField: typing.AbstractSet[_thrift_py3lite_test_included_types.MyEnum]
    setOfStringField: typing.AbstractSet[str]
    def __init__(
        self,
        *,
        setOfI32Field: typing.AbstractSet[int] = ...,
        setOfStructField: typing.AbstractSet[MyStruct] = ...,
        setOfEnumField: typing.AbstractSet[
            _thrift_py3lite_test_included_types.MyEnum
        ] = ...,
        setOfStringField: typing.AbstractSet[str] = ...,
    ) -> None: ...
    def __call__(
        self,
        *,
        setOfI32Field: typing.Optional[typing.AbstractSet[int]] = ...,
        setOfStructField: typing.Optional[typing.AbstractSet[MyStruct]] = ...,
        setOfEnumField: typing.Optional[
            typing.AbstractSet[_thrift_py3lite_test_included_types.MyEnum]
        ] = ...,
        setOfStringField: typing.Optional[typing.AbstractSet[str]] = ...,
    ) -> SetStruct: ...
    def __iter__(
        self,
    ) -> typing.Iterator[
        typing.Tuple[
            str,
            typing.Union[
                typing.AbstractSet[int],
                typing.AbstractSet[MyStruct],
                typing.AbstractSet[_thrift_py3lite_test_included_types.MyEnum],
                typing.AbstractSet[str],
            ],
        ]
    ]: ...

class MapStruct(_fbthrift_py3lite_types.Struct):
    mapOfStrI32Field: typing.Mapping[str, int]
    mapOfStrStructField: typing.Mapping[str, MyStruct]
    mapOfStrEnumField: typing.Mapping[str, _thrift_py3lite_test_included_types.MyEnum]
    def __init__(
        self,
        *,
        mapOfStrI32Field: typing.Mapping[str, int] = ...,
        mapOfStrStructField: typing.Mapping[str, MyStruct] = ...,
        mapOfStrEnumField: typing.Mapping[
            str, _thrift_py3lite_test_included_types.MyEnum
        ] = ...,
    ) -> None: ...
    def __call__(
        self,
        *,
        mapOfStrI32Field: typing.Optional[typing.Mapping[str, int]] = ...,
        mapOfStrStructField: typing.Optional[typing.Mapping[str, MyStruct]] = ...,
        mapOfStrEnumField: typing.Optional[
            typing.Mapping[str, _thrift_py3lite_test_included_types.MyEnum]
        ] = ...,
    ) -> MapStruct: ...
    def __iter__(
        self,
    ) -> typing.Iterator[
        typing.Tuple[
            str,
            typing.Union[
                typing.Mapping[str, int],
                typing.Mapping[str, MyStruct],
                typing.Mapping[str, _thrift_py3lite_test_included_types.MyEnum],
            ],
        ]
    ]: ...

class MyException(_fbthrift_py3lite_exceptions.GeneratedError):
    error_message: str
    internal_error_message: str
    def __init__(
        self,
        *,
        error_message: str = ...,
        internal_error_message: str = ...,
    ) -> None: ...
    def __call__(
        self,
        *,
        error_message: typing.Optional[str] = ...,
        internal_error_message: typing.Optional[str] = ...,
    ) -> MyException: ...

class StructWithDefaults(_fbthrift_py3lite_types.Struct):
    intField: int
    listOfIntField: typing.Sequence[int]
    simpleStructField: MyStruct
    enumField: _thrift_py3lite_test_included_types.MyEnum
    setOfI32Field: typing.AbstractSet[int]
    mapOfStrI32Field: typing.Mapping[str, int]
    stringField: str
    binaryField: bytes
    def __init__(
        self,
        *,
        intField: int = ...,
        listOfIntField: typing.Sequence[int] = ...,
        simpleStructField: MyStruct = ...,
        enumField: _thrift_py3lite_test_included_types.MyEnum = ...,
        setOfI32Field: typing.AbstractSet[int] = ...,
        mapOfStrI32Field: typing.Mapping[str, int] = ...,
        stringField: str = ...,
        binaryField: bytes = ...,
    ) -> None: ...
    def __call__(
        self,
        *,
        intField: typing.Optional[int] = ...,
        listOfIntField: typing.Optional[typing.Sequence[int]] = ...,
        simpleStructField: typing.Optional[MyStruct] = ...,
        enumField: typing.Optional[_thrift_py3lite_test_included_types.MyEnum] = ...,
        setOfI32Field: typing.Optional[typing.AbstractSet[int]] = ...,
        mapOfStrI32Field: typing.Optional[typing.Mapping[str, int]] = ...,
        stringField: typing.Optional[str] = ...,
        binaryField: typing.Optional[bytes] = ...,
    ) -> StructWithDefaults: ...
    def __iter__(
        self,
    ) -> typing.Iterator[
        typing.Tuple[
            str,
            typing.Union[
                int,
                typing.Sequence[int],
                MyStruct,
                _thrift_py3lite_test_included_types.MyEnum,
                typing.AbstractSet[int],
                typing.Mapping[str, int],
                str,
                bytes,
            ],
        ]
    ]: ...

class SimpleStruct(_fbthrift_py3lite_types.Struct):
    intField: int
    def __init__(
        self,
        *,
        intField: int = ...,
    ) -> None: ...
    def __call__(
        self,
        *,
        intField: typing.Optional[int] = ...,
    ) -> SimpleStruct: ...
    def __iter__(
        self,
    ) -> typing.Iterator[typing.Tuple[str, typing.Union[int]]]: ...

class EmptyStruct(_fbthrift_py3lite_types.Struct):
    def __init__(
        self,
    ) -> None: ...
    def __call__(
        self,
    ) -> EmptyStruct: ...
    def __iter__(
        self,
    ) -> typing.Iterator[typing.Tuple[str, None]]: ...
