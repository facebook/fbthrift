#!/usr/bin/env python3
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

import convertible.thrift_types as python_types
import convertible.ttypes as py_legacy_types
import convertible.types as py3_types
from thrift.py3.types import Struct


class PyToPy3ConverterTest(unittest.TestCase):
    def test_simple(self) -> None:
        simple = py_legacy_types.Simple(
            intField=42,
            strField="simple",
            intList=[1, 2, 3],
            strSet={"hello", "world"},
            strToIntMap={"one": 1, "two": 2},
            color=py_legacy_types.Color.GREEN,
            name="myname",
        ).to_py3_struct()
        self.assertEqual(simple.intField, 42)
        self.assertEqual(simple.strField, "simple")
        self.assertEqual(simple.intList, [1, 2, 3])
        self.assertEqual(simple.strSet, {"hello", "world"})
        self.assertEqual(simple.strToIntMap, {"one": 1, "two": 2})
        self.assertEqual(simple.color, py3_types.Color.GREEN)
        self.assertEqual(simple.name_, "myname")

    def test_nested(self) -> None:
        nested = py_legacy_types.Nested(
            simpleField=py_legacy_types.Simple(
                intField=42,
                strField="simple",
                intList=[1, 2, 3],
                strSet={"hello", "world"},
                strToIntMap={"one": 1, "two": 2},
                color=py_legacy_types.Color.NONE,
                name="myname",
            ),
            simpleList=[
                py_legacy_types.Simple(
                    intField=200,
                    strField="face",
                    intList=[4, 5, 6],
                    strSet={"keep", "calm"},
                    strToIntMap={"three": 3, "four": 4},
                    color=py_legacy_types.Color.RED,
                    name="myname",
                ),
                py_legacy_types.Simple(
                    intField=404,
                    strField="b00k",
                    intList=[7, 8, 9],
                    strSet={"carry", "on"},
                    strToIntMap={"five": 5, "six": 6},
                    color=py_legacy_types.Color.GREEN,
                    name="myname",
                ),
            ],
            colorToSimpleMap={
                py_legacy_types.Color.BLUE: py_legacy_types.Simple(
                    intField=500,
                    strField="internal",
                    intList=[10],
                    strSet={"server", "error"},
                    strToIntMap={"seven": 7, "eight": 8, "nine": 9},
                    color=py_legacy_types.Color.BLUE,
                    name="myname",
                )
            },
        ).to_py3_struct()
        self.assertEqual(nested.simpleField.intField, 42)
        self.assertEqual(nested.simpleList[0].intList, [4, 5, 6])
        self.assertEqual(nested.simpleList[1].strSet, {"carry", "on"})
        self.assertEqual(
            nested.colorToSimpleMap[py3_types.Color.BLUE].color, py3_types.Color.BLUE
        )

    def test_simple_union(self) -> None:
        simple_union = py_legacy_types.Union(intField=42).to_py3_struct()
        self.assertEqual(simple_union.type, py3_types.Union.Type.intField)
        self.assertEqual(simple_union.value, 42)

    def test_union_with_py3_name_annotation(self) -> None:
        simple_union = py_legacy_types.Union(name="myname").to_py3_struct()
        self.assertEqual(simple_union.type, py3_types.Union.Type.name_)
        self.assertEqual(simple_union.value, "myname")

    def test_union_with_containers(self) -> None:
        union_with_list = py_legacy_types.Union(intList=[1, 2, 3]).to_py3_struct()
        self.assertEqual(union_with_list.type, py3_types.Union.Type.intList)
        self.assertEqual(union_with_list.value, [1, 2, 3])

    def test_complex_union(self) -> None:
        complex_union = py_legacy_types.Union(
            simpleField=py_legacy_types.Simple(
                intField=42,
                strField="simple",
                intList=[1, 2, 3],
                strSet={"hello", "world"},
                strToIntMap={"one": 1, "two": 2},
                color=py_legacy_types.Color.NONE,
            )
        ).to_py3_struct()
        self.assertEqual(complex_union.type, py3_types.Union.Type.simple_)
        self.assertEqual(complex_union.simple_.intField, 42)

    def test_optional_defaults(self) -> None:
        converted = py_legacy_types.OptionalDefaultsStruct().to_py3_struct()
        # pyre-fixme[6]: Expected `HasIsSet[Variable[thrift.py3.py3_types._T]]` for 1st
        #  param but got `OptionalDefaultsStruct`.
        self.assertFalse(Struct.isset(converted).sillyString)
        # pyre-fixme[6]: Expected `HasIsSet[Variable[thrift.py3.py3_types._T]]` for 1st
        #  param but got `OptionalDefaultsStruct`.
        self.assertFalse(Struct.isset(converted).sillyColor)


class PythontoPy3ConverterTest(unittest.TestCase):
    def test_simple(self) -> None:
        simple = python_types.Simple(
            intField=42,
            strField="simple",
            intList=[1, 2, 3],
            strSet={"hello", "world"},
            strToIntMap={"one": 1, "two": 2},
            color=python_types.Color.GREEN,
            name_="myname",
        ).to_py3_struct()
        self.assertEqual(simple.intField, 42)
        self.assertEqual(simple.strField, "simple")
        self.assertEqual(simple.intList, [1, 2, 3])
        self.assertEqual(simple.strSet, {"hello", "world"})
        self.assertEqual(simple.strToIntMap, {"one": 1, "two": 2})
        self.assertEqual(simple.color, py3_types.Color.GREEN)
        self.assertEqual(simple.name_, "myname")

    def test_nested(self) -> None:
        nested = python_types.Nested(
            simpleField=python_types.Simple(
                intField=42,
                strField="simple",
                intList=[1, 2, 3],
                strSet={"hello", "world"},
                strToIntMap={"one": 1, "two": 2},
                color=python_types.Color.NONE,
                name_="myname",
            ),
            simpleList=[
                python_types.Simple(
                    intField=200,
                    strField="face",
                    intList=[4, 5, 6],
                    strSet={"keep", "calm"},
                    strToIntMap={"three": 3, "four": 4},
                    color=python_types.Color.RED,
                    name_="myname",
                ),
                python_types.Simple(
                    intField=404,
                    strField="b00k",
                    intList=[7, 8, 9],
                    strSet={"carry", "on"},
                    strToIntMap={"five": 5, "six": 6},
                    color=python_types.Color.GREEN,
                    name_="myname",
                ),
            ],
            colorToSimpleMap={
                python_types.Color.BLUE: python_types.Simple(
                    intField=500,
                    strField="internal",
                    intList=[10],
                    strSet={"server", "error"},
                    strToIntMap={"seven": 7, "eight": 8, "nine": 9},
                    color=python_types.Color.BLUE,
                    name_="myname",
                )
            },
        ).to_py3_struct()
        self.assertEqual(nested.simpleField.intField, 42)
        self.assertEqual(nested.simpleList[0].intList, [4, 5, 6])
        self.assertEqual(nested.simpleList[1].strSet, {"carry", "on"})
        self.assertEqual(
            nested.colorToSimpleMap[py3_types.Color.BLUE].color, py3_types.Color.BLUE
        )

    def test_simple_union(self) -> None:
        simple_union = python_types.Union(intField=42).to_py3_struct()
        self.assertEqual(simple_union.type, py3_types.Union.Type.intField)
        self.assertEqual(simple_union.value, 42)

    def test_union_with_py3_name_annotation(self) -> None:
        simple_union = python_types.Union(name_="myname").to_py3_struct()
        self.assertEqual(simple_union.type, py3_types.Union.Type.name_)
        self.assertEqual(simple_union.value, "myname")

    def test_union_with_containers(self) -> None:
        union_with_list = python_types.Union(intList=[1, 2, 3]).to_py3_struct()
        self.assertEqual(union_with_list.type, py3_types.Union.Type.intList)
        self.assertEqual(union_with_list.value, [1, 2, 3])

    def test_complex_union(self) -> None:
        complex_union = python_types.Union(
            simple_=python_types.Simple(
                intField=42,
                strField="simple",
                intList=[1, 2, 3],
                strSet={"hello", "world"},
                strToIntMap={"one": 1, "two": 2},
                color=python_types.Color.NONE,
            )
        ).to_py3_struct()
        self.assertEqual(complex_union.type, py3_types.Union.Type.simple_)
        self.assertEqual(complex_union.simple_.intField, 42)

    def test_optional_defaults(self) -> None:
        converted = python_types.OptionalDefaultsStruct().to_py3_struct()
        # pyre-fixme[6]: Expected `HasIsSet[Variable[thrift.py3.py3_types._T]]` for 1st
        #  param but got `OptionalDefaultsStruct`.
        self.assertFalse(Struct.isset(converted).sillyString)
        # pyre-fixme[6]: Expected `HasIsSet[Variable[thrift.py3.py3_types._T]]` for 1st
        #  param but got `OptionalDefaultsStruct`.
        self.assertFalse(Struct.isset(converted).sillyColor)
