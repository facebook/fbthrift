# Copyright (c) Meta Platforms, Inc. and affiliates.
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

import unittest

from testing.example.types import (
    Color,
    ErrorWithEnum,
    ErrorWithMessageAnnotation,
    SimpleError,
    TestNestedStruct,
    TestStruct,
    TestStructSimple,
    TestStructWithList,
    TestStructWithMixin,
    TestStructWithRefAnnotations,
    TestStructWithSet,
)


class NoLegacyTest(unittest.TestCase):
    def test_enum(self) -> None:
        color = Color.red
        self.assertEqual(color, Color.red)

    def test_simple_exception(self) -> None:
        try:
            raise SimpleError(errortext="testerror", retcode=-1)
        except SimpleError as exception:
            self.assertEqual(exception.errortext, "testerror")
            self.assertEqual(exception.retcode, -1)
        else:
            self.assertFalse("Should not get here")

    def test_exception_with_enum(self) -> None:
        try:
            raise ErrorWithEnum(color=Color.green, retcode=0)
        except ErrorWithEnum as exception:
            self.assertEqual(exception.color, Color.green)
            self.assertEqual(exception.retcode, 0)
        else:
            self.assertFalse("Should not get here")

    def test_exception_without_message_annotation(self) -> None:
        try:
            raise SimpleError(errortext="test message", retcode=-1)
        except SimpleError as exception:
            message = str(exception)
            self.assertEqual(message, "('test message', -1)")
        else:
            self.assertFalse("Should not get here")

    def test_exception_with_message_annotation(self) -> None:
        try:
            raise ErrorWithMessageAnnotation(errortext="test message", retcode=-1)
        except ErrorWithMessageAnnotation as exception:
            message = str(exception)
            self.assertEqual(message, "test message")
        else:
            self.assertFalse("Should not get here")

    def test_struct_with_all_fields_set(self) -> None:
        testStruct = TestStruct(
            field1="field1", field2="field2", field3=Color.blue, field4=Color.green
        )
        self.assertEqual(testStruct.field1, "field1")
        self.assertEqual(testStruct.field2, "field2")
        self.assertEqual(testStruct.field3, Color.blue)
        self.assertEqual(testStruct.field4, Color.green)

    def test_struct_with_optional_not_set(self) -> None:
        testStruct = TestStruct(field1="field1", field3=Color.green)
        self.assertEqual(testStruct.field1, "field1")
        self.assertIsNone(testStruct.field2)
        self.assertEqual(testStruct.field3, Color.green)
        self.assertIsNone(testStruct.field4)

    def test_struct_with_default_args(self) -> None:
        testStruct = TestStruct()
        self.assertEqual(testStruct.field1, "")
        self.assertIsNone(testStruct.field2)
        self.assertEqual(testStruct.field3, Color.red)
        self.assertIsNone(testStruct.field4)

    def test_struct_with_list(self) -> None:
        testStruct = TestStructWithList(numbers=[1, 2, 3])
        self.assertEquals(testStruct.numbers, [1, 2, 3])

    def test_struct_with_set(self) -> None:
        testStruct = TestStructWithSet(numbers={1, 2, 3})
        self.assertEqual(len(testStruct.numbers), 3)
        for i in [1, 2, 3]:
            self.assertTrue(i in testStruct.numbers)

    def test_struct_with_box_annotation(self) -> None:
        testStruct = TestStructWithRefAnnotations()
        self.assertIsNone(testStruct.data1)

    def test_struct_with_valid_ptr(self) -> None:
        testStruct = TestStructWithRefAnnotations(data1=[1, 2, 3])
        self.assertEqual(testStruct.data1, [1, 2, 3])

    def test_nested_struct(self) -> None:
        testStruct = TestNestedStruct(nested=TestStructWithList(numbers=[1, 2, 3]))
        self.assertEqual(testStruct.nested.numbers, [1, 2, 3])

    def test_mixin_struct(self) -> None:
        testStruct = TestStructWithMixin(
            field3="field3", field4=TestStructSimple(field1="field1", field2=5)
        )
        self.assertEqual(testStruct.field3, "field3")
        self.assertEqual(testStruct.field1, "field1")
        self.assertEqual(testStruct.field2, 5)
