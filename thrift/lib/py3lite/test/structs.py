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
import math
import unittest

from testing.lite_types import (
    Color,
    Integers,
    File,
    OptionalFile,
    Kind,
    Nested1,
    Nested2,
    Nested3,
    Optionals,
    Reserved,
    Runtime,
    UnusedError,
    easy,
    numerical,
)
from thrift.py3lite.serializer import (
    deserialize,
    serialize_iobuf,
)
from thrift.py3lite.types import (
    isset,
    update_nested_field,
)


class StructTests(unittest.TestCase):
    def test_isset_Struct(self) -> None:
        to_serialize = OptionalFile(name="/dev/null", type=8)
        serialized = serialize_iobuf(to_serialize)
        file = deserialize(File, serialized)
        self.assertTrue(isset(file)["type"])
        self.assertFalse(isset(file)["permissions"])

        to_serialize = OptionalFile(name="/dev/null")
        serialized = serialize_iobuf(to_serialize)
        file = deserialize(File, serialized)
        self.assertEqual(file.type, Kind.REGULAR)
        self.assertFalse(isset(file)["type"])

    def test_isset_Union(self) -> None:
        i = Integers(large=2)
        with self.assertRaises(TypeError):
            # pyre-ignore[6]: for test
            isset(i)["large"]

    def test_isset_Error(self) -> None:
        e = UnusedError(message="ACK")
        self.assertTrue(isset(e)["message"])

    def test_copy(self) -> None:
        x = easy(val=1, an_int=Integers(small=300), name="foo", val_list=[1, 2, 3, 4])
        dif_list = copy.copy(x.val_list)
        self.assertEqual(x.val_list, dif_list)
        dif_int = copy.copy(x.an_int)
        self.assertEqual(x.an_int, dif_int)

    def test_hashability(self) -> None:
        hash(easy())

    def test_optional_struct_creation(self) -> None:
        with self.assertRaises(TypeError):
            # pyre-ignore[19]: for test
            easy(1, [1, 1], "test", Integers(tiny=1))
        easy(val=1, an_int=Integers(small=500))
        with self.assertRaises(TypeError):
            # pyre-ignore[6]: for test
            easy(name=b"binary")
        # Only Required Fields don't accept None
        easy(val=5, an_int=None)

    def test_call_replace(self) -> None:
        x = easy(val=1, an_int=Integers(small=300), name="foo")
        y = x(name="bar")
        self.assertNotEqual(x.name, y.name)
        z = y(an_int=None, val=4)
        self.assertNotEqual(x.an_int, z.an_int)
        self.assertNotEqual(x.val, z.val)
        self.assertIsNone(z.an_int.value)
        self.assertEqual(y.val, x.val)
        self.assertEqual(y.an_int, x.an_int)
        x = easy()
        self.assertIsNotNone(x.val)
        self.assertIsNotNone(x.val_list)
        self.assertIsNone(x.name)
        self.assertIsNotNone(x.an_int)

    def test_call_replace_container(self) -> None:
        x = Optionals(values=["a", "b", "c"])
        z = x(values=["b", "c"])
        y = z(values=None)
        self.assertIsNone(y.values)

    def test_runtime_checks(self) -> None:
        x = Runtime()
        with self.assertRaises(TypeError):
            # pyre-ignore[6]: for test
            x(bool_val=5)

        with self.assertRaises(TypeError):
            # pyre-ignore[6]: for test
            Runtime(bool_val=5)

        with self.assertRaises(TypeError):
            # pyre-ignore[6]: for test
            x(enum_val=2)

        with self.assertRaises(TypeError):
            # pyre-ignore[6]: for test
            Runtime(enum_val=2)

        with self.assertRaises(TypeError):
            # pyre-ignore[6]: for test
            x(int_list_val=["foo", "bar", "baz"])

        with self.assertRaises(TypeError):
            # pyre-ignore[6]: for test
            Runtime(int_list_val=["foo", "bar", "baz"])

    def test_reserved(self) -> None:
        x = Reserved(
            from_="hello",
            nonlocal_=3,
            ok="bye",
            is_cpdef=True,
            move="Qh4xe1",
            inst="foo",
            changes="bar",
        )
        self.assertEqual(x.from_, "hello")
        self.assertEqual(x.nonlocal_, 3)
        self.assertEqual(x.ok, "bye")
        self.assertEqual(x.is_cpdef, True)
        self.assertEqual(x.move, "Qh4xe1")
        self.assertEqual(x.inst, "foo")
        self.assertEqual(x.changes, "bar")

    def test_ordering(self) -> None:
        x = Runtime(bool_val=False, enum_val=Color.red, int_list_val=[64, 128])
        y = x(bool_val=True)
        self.assertLess(x, y)
        self.assertLessEqual(x, y)
        self.assertGreater(y, x)
        self.assertGreaterEqual(y, x)
        self.assertEqual([x, y], sorted([y, x]))

    def test_init_with_invalid_field(self) -> None:
        with self.assertRaises(TypeError):
            # pyre-ignore[28]: for test
            easy(val=1, an_int=Integers(small=300), name="foo", val_lists=[1, 2, 3, 4])

    def test_iterate(self) -> None:
        x = Reserved(
            from_="hello",
            nonlocal_=3,
            ok="bye",
            is_cpdef=True,
            move="Qh4xe1",
            inst="foo",
            changes="bar",
        )
        self.assertEqual(
            list(x),
            [
                ("from_", "hello"),
                ("nonlocal_", 3),
                ("ok", "bye"),
                ("is_cpdef", True),
                ("move", "Qh4xe1"),
                ("inst", "foo"),
                ("changes", "bar"),
            ],
        )

    def test_update_nested_fields(self) -> None:
        n = Nested1(a=Nested2(b=Nested3(c=easy(val=42, name="foo"))))
        n = update_nested_field(n, {"a.b.c": easy(val=128)})
        self.assertEqual(n.a.b.c.val, 128)

    def test_update_multiple_nested_fields(self) -> None:
        n = Nested1(a=Nested2(b=Nested3(c=easy(val=42, name="foo"))))
        n = update_nested_field(
            n,
            {
                "a.b.c.name": "bar",
                "a.b.c.val": 256,
            },
        )
        self.assertEqual(n.a.b.c.name, "bar")
        self.assertEqual(n.a.b.c.val, 256)

    def test_update_invalid_nested_fields(self) -> None:
        n = Nested1(a=Nested2(b=Nested3(c=easy(val=42, name="foo"))))
        with self.assertRaises(ValueError):
            update_nested_field(n, {"": 0})
        with self.assertRaises(ValueError):
            update_nested_field(n, {"e": 0})
        with self.assertRaises(ValueError):
            update_nested_field(n, {"a.b.e": 0})
        with self.assertRaises(ValueError):
            update_nested_field(n, {"a.e.f": 0})

    def test_update_conflicting_nested_fields(self) -> None:
        n = Nested1(a=Nested2(b=Nested3(c=easy(val=42, name="foo"))))
        with self.assertRaises(ValueError):
            n = update_nested_field(
                n,
                {
                    "a.b.c": easy(val=128),
                    "a.b.c.val": 256,
                },
            )


class NumericalConversionsTests(unittest.TestCase):
    def test_overflow(self) -> None:
        with self.assertRaises(OverflowError):
            numerical(float_val=5, int_val=2 ** 63 - 1)

        with self.assertRaises(OverflowError):
            numerical(float_val=5, int_val=2, int_list=[5, 2 ** 32])

    def test_int_to_float(self) -> None:
        x = numerical(int_val=5, float_val=5, float_list=[1, 5, 6])
        x(float_val=10)
        x(float_list=[6, 7, 8])

    def test_int_to_i64(self) -> None:
        large = 2 ** 63 - 1
        numerical(int_val=5, float_val=5, i64_val=int(large))
        too_large = 2 ** 65 - 1
        with self.assertRaises(OverflowError):
            numerical(int_val=5, float_val=5, i64_val=int(too_large))

    def test_float_to_int_required_field(self) -> None:
        with self.assertRaises(TypeError):
            # pyre-ignore[6]: for test
            numerical(int_val=math.pi, float_val=math.pi)

    def test_float_to_int_unqualified_field(self) -> None:
        with self.assertRaises(TypeError):
            numerical(
                float_val=math.pi,
                # pyre-ignore[6]: for test
                int_val=math.pi,
            )

    def test_float_to_int_list(self) -> None:
        with self.assertRaises(TypeError):
            numerical(
                int_val=5,
                float_val=math.pi,
                # pyre-ignore[6]: for test
                int_list=[math.pi, math.e],
            )
