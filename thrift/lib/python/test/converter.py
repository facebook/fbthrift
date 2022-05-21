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
from thrift.python.converter import to_python_struct


class Py3toPythonConverterTest(unittest.TestCase):
    def test_simple(self) -> None:
        simple = py3_types.Simple(
            intField=42,
            strField="simple",
            intList=[1, 2, 3],
            strSet={"hello", "world"},
            strToIntMap={"one": 1, "two": 2},
            color=py3_types.Color.GREEN,
            name_="myname",
        ).to_python_struct()
        self.assertEqual(simple.intField, 42)
        self.assertEqual(simple.strField, "simple")
        self.assertEqual(simple.intList, [1, 2, 3])
        self.assertEqual(simple.strSet, {"hello", "world"})
        self.assertEqual(simple.strToIntMap, {"one": 1, "two": 2})
        self.assertEqual(simple.color, python_types.Color.GREEN)
        self.assertEqual(simple.name_, "myname")

    def test_nested(self) -> None:
        nested = py3_types.Nested(
            simpleField=py3_types.Simple(
                intField=42,
                strField="simple",
                intList=[1, 2, 3],
                strSet={"hello", "world"},
                strToIntMap={"one": 1, "two": 2},
                color=py3_types.Color.NONE,
                name_="myname",
            ),
            simpleList=[
                py3_types.Simple(
                    intField=200,
                    strField="face",
                    intList=[4, 5, 6],
                    strSet={"keep", "calm"},
                    strToIntMap={"three": 3, "four": 4},
                    color=py3_types.Color.RED,
                    name_="myname",
                ),
                py3_types.Simple(
                    intField=404,
                    strField="b00k",
                    intList=[7, 8, 9],
                    strSet={"carry", "on"},
                    strToIntMap={"five": 5, "six": 6},
                    color=py3_types.Color.GREEN,
                    name_="myname",
                ),
            ],
            colorToSimpleMap={
                py3_types.Color.BLUE: py3_types.Simple(
                    intField=500,
                    strField="internal",
                    intList=[10],
                    strSet={"server", "error"},
                    strToIntMap={"seven": 7, "eight": 8, "nine": 9},
                    color=py3_types.Color.BLUE,
                    name_="myname",
                )
            },
        ).to_python_struct()
        self.assertEqual(nested.simpleField.intField, 42)
        self.assertEqual(nested.simpleList[0].intList, [4, 5, 6])
        self.assertEqual(nested.simpleList[1].strSet, {"carry", "on"})
        self.assertEqual(
            nested.colorToSimpleMap[python_types.Color.BLUE].color,
            python_types.Color.BLUE,
        )

    def test_simple_union(self) -> None:
        simple_union = py3_types.Union(intField=42).to_python_struct()
        self.assertEqual(simple_union.type, python_types.Union.Type.intField)
        self.assertEqual(simple_union.value, 42)

    def test_union_with_py3_name_annotation(self) -> None:
        simple_union = py3_types.Union(name_="myname").to_python_struct()
        self.assertEqual(simple_union.type, python_types.Union.Type.name_)
        self.assertEqual(simple_union.value, "myname")

    def test_union_with_containers(self) -> None:
        union_with_list = py3_types.Union(intList=[1, 2, 3]).to_python_struct()
        self.assertEqual(union_with_list.type, python_types.Union.Type.intList)
        self.assertEqual(union_with_list.value, [1, 2, 3])

    def test_complex_union(self) -> None:
        complex_union = py3_types.Union(
            simple_=py3_types.Simple(
                intField=42,
                strField="simple",
                intList=[1, 2, 3],
                strSet={"hello", "world"},
                strToIntMap={"one": 1, "two": 2},
                color=py3_types.Color.NONE,
            )
        ).to_python_struct()
        self.assertEqual(complex_union.type, python_types.Union.Type.simple_)
        self.assertEqual(complex_union.simple_.intField, 42)


class PytoPy3liteConverterTest(unittest.TestCase):
    def test_simple(self) -> None:
        simple = to_python_struct(
            python_types.Simple,
            py_legacy_types.Simple(
                intField=42,
                strField="simple",
                intList=[1, 2, 3],
                strSet={"hello", "world"},
                strToIntMap={"one": 1, "two": 2},
                color=py_legacy_types.Color.GREEN,
                name="myname",
            ),
        )
        self.assertEqual(simple.intField, 42)
        self.assertEqual(simple.strField, "simple")
        self.assertEqual(simple.intList, [1, 2, 3])
        self.assertEqual(simple.strSet, {"hello", "world"})
        self.assertEqual(simple.strToIntMap, {"one": 1, "two": 2})
        self.assertEqual(simple.color, python_types.Color.GREEN)
        self.assertEqual(simple.name_, "myname")

    def test_nested(self) -> None:
        nested = to_python_struct(
            python_types.Nested,
            py_legacy_types.Nested(
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
            ),
        )
        self.assertEqual(nested.simpleField.intField, 42)
        self.assertEqual(nested.simpleList[0].intList, [4, 5, 6])
        self.assertEqual(nested.simpleList[1].strSet, {"carry", "on"})
        self.assertEqual(
            nested.colorToSimpleMap[python_types.Color.BLUE].color,
            python_types.Color.BLUE,
        )

    def test_simple_union(self) -> None:
        simple_union = to_python_struct(
            python_types.Union, py_legacy_types.Union(intField=42)
        )
        self.assertEqual(simple_union.type, python_types.Union.Type.intField)
        self.assertEqual(simple_union.value, 42)

    def test_union_with_py3_name_annotation(self) -> None:
        simple_union = to_python_struct(
            python_types.Union, py_legacy_types.Union(name="myname")
        )
        self.assertEqual(simple_union.type, python_types.Union.Type.name_)
        self.assertEqual(simple_union.value, "myname")

    def test_union_with_containers(self) -> None:
        union_with_list = to_python_struct(
            python_types.Union, py_legacy_types.Union(intList=[1, 2, 3])
        )
        self.assertEqual(union_with_list.type, python_types.Union.Type.intList)
        self.assertEqual(union_with_list.value, [1, 2, 3])

    def test_complex_union(self) -> None:
        complex_union = to_python_struct(
            python_types.Union,
            py_legacy_types.Union(
                simpleField=py_legacy_types.Simple(
                    intField=42,
                    strField="simple",
                    intList=[1, 2, 3],
                    strSet={"hello", "world"},
                    strToIntMap={"one": 1, "two": 2},
                    color=py_legacy_types.Color.NONE,
                )
            ),
        )
        self.assertEqual(complex_union.type, python_types.Union.Type.simple_)
        self.assertEqual(complex_union.simple_.intField, 42)
