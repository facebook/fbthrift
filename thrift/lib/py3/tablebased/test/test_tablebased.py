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
from types import MappingProxyType
from unittest import TestCase

from thrift.py3lite.serializer import serialize_iobuf, deserialize
from thrift.py3lite.test.included.types import MyEnum, IncludedStruct
from thrift.py3lite.test.module.types import (
    MyStruct,
    AnotherStruct,
    RecursiveStruct,
    MyUnion,
    StructWithAUnion,
    MapStruct,
    PrimitiveStruct,
    StringStruct,
    SetStruct,
    MyException,
    StructWithDefaults,
    SimpleStruct,
    EmptyStruct,
)
from thrift.py3lite.types import BadEnum


class TablebasedTest(TestCase):
    def test_simple_struct(self) -> None:
        s1 = MyStruct(
            intField=456,
            listOfIntField=[34, 59, 28],
            enumField=MyEnum.TWO,
        )
        serialized = serialize_iobuf(s1)
        s2 = deserialize(MyStruct, serialized)
        self.assertIsNot(s1, s2)
        self.assertIs(s2.enumField, MyEnum.TWO)
        self.assertEqual(s1, s2)
        self.assertIsNone(s2.optionalField)

    def test_complex_struct(self) -> None:
        included = IncludedStruct(intField=456, listOfIntField=[34, 59, 28])
        s1 = MyStruct(intField=456, listOfIntField=[34, 59, 28], enumField=MyEnum.ONE)
        s2 = AnotherStruct(
            structField=included, listOfStructField=[s1], listOflistOfStructField=[[s1]]
        )
        serialized = serialize_iobuf(s2)
        s3 = deserialize(AnotherStruct, serialized)
        self.assertIsNot(s3, s2)
        self.assertEqual(s3, s2)
        self.assertIsNot(s3.structField, included)
        self.assertEqual(s3.structField, included)
        self.assertEqual(s3.listOfStructField, (s1,))
        self.assertEqual(s3.listOflistOfStructField, ((s1,),))

    def test_union(self) -> None:
        type_enum_pairs = [(v.value, v.name) for v in MyUnion.Type]
        self.assertEqual(
            type_enum_pairs,
            [
                (0, "EMPTY"),
                (2, "intField"),
                (3, "MyStructField"),
                (4, "AnotherStructField"),
            ],
        )
        s1 = MyUnion(intField=42)
        self.assertIs(s1.type, MyUnion.Type.intField)
        self.assertEqual(s1.value, 42)
        s2 = StructWithAUnion(unionField=s1)
        serialized = serialize_iobuf(s2)

        s3 = deserialize(StructWithAUnion, serialized)
        self.assertEqual(s2, s3)

        self.assertTrue(s1)
        s4 = MyUnion()
        self.assertFalse(s4)

    def test_primitive_types(self) -> None:
        s1 = PrimitiveStruct(
            boolField=True,
            byteField=127,
            i16Field=32767,
            i32Field=2147483647,
            i64Field=9223372036854775807,
            doubleField=4.56,
            floatField=123.0,
        )
        serialized = serialize_iobuf(s1)
        s2 = deserialize(PrimitiveStruct, serialized)
        self.assertEqual(s1, s2)

    def test_string_types(self) -> None:
        s1 = StringStruct(
            stringField="a string",
            binaryField=b"some bytes",
        )
        serialized = serialize_iobuf(s1)
        s2 = deserialize(StringStruct, serialized)
        self.assertEqual(s2.stringField, "a string")
        self.assertEqual(s2.binaryField, b"some bytes")

    def test_set_type(self) -> None:
        s1 = MyStruct(intField=456, listOfIntField=[34, 59, 28], enumField=MyEnum.ONE)
        s2 = MyStruct(intField=123, listOfIntField=[12, 23], enumField=MyEnum.TWO)
        s3 = SetStruct(
            setOfI32Field={1, 2, 3, 4},
            setOfStructField={s1, s2},
            setOfEnumField={MyEnum.TWO, MyEnum.ONE},
            setOfStringField={"hello", "world"},
        )
        serialized = serialize_iobuf(s3)
        s4 = deserialize(SetStruct, serialized)
        self.assertEqual(s3, s4)

    def test_map_type(self) -> None:
        s1 = MyStruct(intField=456, listOfIntField=[34, 59, 28], enumField=MyEnum.ONE)
        s2 = MyStruct(intField=123, listOfIntField=[12, 23], enumField=MyEnum.TWO)
        s3 = MapStruct(
            mapOfStrI32Field={"a": 1, "b": 2, "c": 3, "d": 4},
            mapOfStrStructField={"a": s1, "b": s2},
            mapOfStrEnumField={"a": MyEnum.TWO, "c": MyEnum.ONE},
        )
        serialized = serialize_iobuf(s3)
        s4 = deserialize(MapStruct, serialized)
        self.assertEqual(s3, s4)

    def test_default_construction(self) -> None:
        my_struct = MyStruct()
        self.assertEqual(my_struct.intField, 0)  # int field defaults to 0
        self.assertEqual(
            my_struct.listOfIntField, ()
        )  # list field defaults to empty tuple
        self.assertEqual(
            my_struct.optionalField, None
        )  # optional field defaults to None
        self.assertEqual(
            my_struct.enumField, BadEnum(MyEnum, 0)
        )  # enum field defaults to 0 enum or BadEnum
        another_struct = AnotherStruct()
        self.assertEqual(
            another_struct.structField, IncludedStruct()
        )  # struct field defaults to default-constructed struct
        struct_with_union = StructWithAUnion()
        union_field = struct_with_union.unionField
        # union field defaults to empty union
        self.assertEqual(union_field.type, MyUnion.Type.EMPTY)
        self.assertIsNone(union_field.value)
        primitive_struct = PrimitiveStruct()
        self.assertEqual(primitive_struct.boolField, False)
        self.assertEqual(primitive_struct.floatField, 0.0)
        self.assertEqual(primitive_struct.doubleField, 0.0)
        self.assertEqual(primitive_struct.byteField, 0)
        self.assertEqual(primitive_struct.i16Field, 0)
        self.assertEqual(primitive_struct.i64Field, 0)
        string_struct = StringStruct()
        self.assertEqual(string_struct.stringField, "")
        self.assertEqual(string_struct.binaryField, b"")
        self.assertEqual(SetStruct().setOfI32Field, frozenset())
        self.assertEqual(MapStruct().mapOfStrStructField, MappingProxyType({}))

    def test_call_replace(self) -> None:
        s1 = MyStruct(intField=456, listOfIntField=[34, 59, 28], enumField=MyEnum.ONE)
        s2 = s1(intField=678, enumField=MyEnum.TWO)
        self.assertEqual(s2.intField, 678)
        self.assertEqual(s2.listOfIntField, (34, 59, 28))
        self.assertIsNone(s2.optionalField)
        self.assertEqual(s2.enumField, MyEnum.TWO)
        s3 = s2(optionalField=(1, 2, 3))
        self.assertEqual(s3.optionalField, (1, 2, 3))
        s4 = s3(optionalField=None, listOfIntField=None, enumField=None)
        self.assertEqual(s3.intField, 678)
        self.assertIsNone(s4.optionalField)
        self.assertEqual(s4.listOfIntField, ())
        self.assertEqual(s4.enumField, BadEnum(MyEnum, 0))
        s5 = AnotherStruct()
        included = IncludedStruct(intField=678)
        s6 = s5(structField=included, listOfStructField=[s1, s2])
        self.assertEqual(s6.structField, included)
        self.assertEqual(s6.listOfStructField, (s1, s2))
        self.assertEqual(s6.listOflistOfStructField, ())

    def test_copy(self) -> None:
        s1 = MyStruct(intField=456, listOfIntField=[34, 59, 28], enumField=MyEnum.ONE)
        s2 = copy.copy(s1)
        self.assertEqual(s1, s2)
        self.assertIsNot(s1, s2)

        u1 = MyUnion(intField=42)
        u2 = copy.copy(u1)
        self.assertEqual(u1, u2)
        self.assertIsNot(u1, u2)

        e1 = MyException(error_message="error", internal_error_message="internal error")
        e2 = copy.copy(e1)
        self.assertEqual(e1, e2)
        self.assertIsNot(e1, e2)

    def test_iter(self) -> None:
        s1 = MyStruct(
            intField=456,
            listOfIntField=[34, 59, 28],
            enumField=MyEnum.TWO,
        )
        self.assertEqual(
            list(s1),
            [
                ("intField", 456),
                ("listOfIntField", (34, 59, 28)),
                ("optionalField", None),
                ("enumField", MyEnum.TWO),
            ],
        )

    def test_recursive(self) -> None:
        s1 = RecursiveStruct(
            recursiveField=RecursiveStruct(), AnotherStructField=AnotherStruct()
        )
        s2 = serialize_iobuf(s1)
        s3 = deserialize(RecursiveStruct, s2)
        self.assertEqual(s1, s3)

    def test_exception(self) -> None:
        ex = MyException(error_message="error", internal_error_message="internal error")
        self.assertEqual(ex.error_message, "error")
        self.assertEqual(ex.internal_error_message, "internal error")
        self.assertEqual(str(ex), "internal error")

    def test_default_values(self) -> None:
        s1 = StructWithDefaults()
        self.assertEqual(s1.intField, 42)
        self.assertEqual(s1.listOfIntField, (12, 34))
        self.assertEqual(
            s1.simpleStructField,
            SimpleStruct(intField=678),
        )
        self.assertEqual(s1.enumField, MyEnum.TWO)
        self.assertEqual(s1.setOfI32Field, {24, 65, 99})
        self.assertEqual(s1.mapOfStrI32Field, {"a": 24, "b": 99})
        self.assertEqual(s1.stringField, "awesome string")
        self.assertEqual(s1.binaryField, b"awesome bytes")
        serialized = serialize_iobuf(EmptyStruct())
        # deserialize from empty payload
        s2 = deserialize(StructWithDefaults, serialized)
        self.assertEqual(s2.intField, 42)
        self.assertEqual(s2.listOfIntField, (12, 34))
        self.assertEqual(
            s2.simpleStructField,
            SimpleStruct(intField=678),
        )
        self.assertEqual(s2.enumField, MyEnum.TWO)
        self.assertEqual(s2.setOfI32Field, {24, 65, 99})
        self.assertEqual(s2.mapOfStrI32Field, {"a": 24, "b": 99})
        self.assertEqual(s2.stringField, "awesome string")
        self.assertEqual(s2.binaryField, b"awesome bytes")
