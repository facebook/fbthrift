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

# pyre-strict


import unittest
from typing import Generator, Type

import testing.thrift_types as python_types
import testing.types as py3_types
from thrift.py3.serializer import serialize as py3_serialize
from thrift.python.serializer import deserialize, Protocol


class Py3CompatibilityTest(unittest.TestCase):
    def test_init_python_struct_with_py3_union(self) -> None:
        py3_integers = py3_types.Integers(small=2023)
        # pyre-fixme[6]: In call `python_types.easy.__init__`, for argument `an_int`, expected `Optional[thrift_types.Integers]` but got `Integers`.
        python_easy = python_types.easy(an_int=py3_integers)
        self.assertEqual(2023, python_easy.an_int.small)

    def test_update_python_struct_with_py3_union(self) -> None:
        python_easy = python_types.easy()
        py3_integers = py3_types.Integers(small=2023)
        # pyre-fixme[6]: In call `python_types.easy.__init__`, for argument `an_int`, expected `Optional[thrift_types.Integers]` but got `Integers`.
        python_easy = python_easy(an_int=py3_integers)
        self.assertEqual(2023, python_easy.an_int.small)

    def test_init_python_struct_with_py3_enum(self) -> None:
        python_file = python_types.File(
            permissions=(py3_types.Perm.read | py3_types.Perm.read),
            type=py3_types.Kind.FIFO,
        )
        self.assertEqual(
            python_types.Perm.read | python_types.Perm.read,
            python_file.permissions,
        )
        self.assertEqual(python_types.Kind.FIFO, python_file.type)

    def test_update_python_struct_with_py3_enum(self) -> None:
        python_file = python_types.File()
        python_file = python_file(
            permissions=(py3_types.Perm.read | py3_types.Perm.read),
            type=py3_types.Kind.FIFO,
        )
        self.assertEqual(
            python_types.Perm.read | python_types.Perm.read,
            python_file.permissions,
        )
        self.assertEqual(python_types.Kind.FIFO, python_file.type)

    def test_init_python_union_with_py3_struct(self) -> None:
        py3_easy = py3_types.easy(name="foo")
        # pyre-fixme[6]: In call `python_types.ComplexUnion.__init__`, for argument `easy_struct`, expected `Optional[thrift_types.easy]` but got `easy`.
        python_complex_union = python_types.ComplexUnion(easy_struct=py3_easy)
        self.assertEqual("foo", python_complex_union.easy_struct.name)

    def test_init_python_union_with_py3_exception(self) -> None:
        py3_error = py3_types.HardError(errortext="oops", code=404)
        # pyre-ignore[6]: In call `python_types.ValueOrError.__init__`, for 1st positional argument, expected `Optional[thrift_types.HardError]` but got `HardError`.
        python_union = python_types.ValueOrError(error=py3_error)
        self.assertEqual(python_union.type, python_types.ValueOrError.Type.error)
        self.assertIsInstance(python_union.error, python_types.HardError)
        self.assertEqual(python_union.error, py3_error._to_python())

    def test_init_python_struct_with_py3_exception(self) -> None:
        py3_error = py3_types.HardError(errortext="oops", code=404)
        # pyre-ignore[6]: In call `python_types.NestedError.__init__`, for 1st positional argument, expected `Optional[thrift_types.HardError]` but got `HardError`.
        python_struct = python_types.NestedError(val_error=py3_error)
        self.assertIsInstance(python_struct.val_error, python_types.HardError)
        self.assertEqual(python_struct.val_error, py3_error._to_python())


class DeserializationCompatibilityTest(unittest.TestCase):
    def assert_struct_ordering(
        self, py3: py3_types.StructOrderRandom | python_types.StructOrderSorted
    ) -> None:
        expected = python_types.StructOrderRandom(**dict(py3))
        for proto in [Protocol.BINARY, Protocol.COMPACT, Protocol.JSON]:
            py3_stored = py3_serialize(py3, protocol=proto)
            python = deserialize(
                python_types.StructOrderRandom, py3_stored, protocol=proto
            )
            self.assertEqual(python, expected, f"Protocol {proto}")

    def gen_structs(
        self,
        kls: Type[py3_types.StructOrderRandom] | Type[python_types.StructOrderSorted],
    ) -> Generator[
        py3_types.StructOrderRandom | python_types.StructOrderSorted, None, None
    ]:
        yield kls(
            a=0,
            b="0",
            c=False,
            d=True,
        )
        yield kls(
            a=3,
            b="1234",
            c=False,
            d=True,
        )
        yield kls(
            a=1,
            b="0",
            c=False,
            d=True,
        )

    # Test that deserialize is insensitive to struct field declaration order.
    # This struct has different field orders in py3, which uses declaration order,
    # and in python, which uses id/key order. This test verifies that python deserialization
    # is insensitive to the order of the fields in the serialized data.
    def test_py3_migration_struct_ordering(self) -> None:
        for struct in self.gen_structs(py3_types.StructOrderRandom):
            self.assert_struct_ordering(struct)

    # Test that would verify that python deserialization is insensitive to the order of the fields in the serialized data.
    # It is not a *live* test until a diff that actually implements change from id-key order to declaration order.
    def test_python_struct_ordering(self) -> None:
        for struct in self.gen_structs(python_types.StructOrderSorted):
            self.assert_struct_ordering(struct)
