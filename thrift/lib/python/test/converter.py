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

import convertible.thrift_types as thrift_types
import convertible.ttypes as ttypes
import convertible.types as types
from thrift.python.converter import to_python_struct


class Py3toPythonConverterTest(unittest.TestCase):
    def test_simple(self) -> None:
        simple = to_python_struct(
            thrift_types.Simple,
            types.Simple(
                intField=42,
                strField="simple",
                intList=[1, 2, 3],
                strSet={"hello", "world"},
                strToIntMap={"one": 1, "two": 2},
                color=types.Color.GREEN,
                name_="myname",
            ),
        )
        self.assertEqual(simple.intField, 42)
        self.assertEqual(simple.strField, "simple")
        self.assertEqual(simple.intList, [1, 2, 3])
        self.assertEqual(simple.strSet, {"hello", "world"})
        self.assertEqual(simple.strToIntMap, {"one": 1, "two": 2})
        self.assertEqual(simple.color, thrift_types.Color.GREEN)
        self.assertEqual(simple.name_, "myname")

    def test_nested(self) -> None:
        nested = to_python_struct(
            thrift_types.Nested,
            types.Nested(
                simpleField=types.Simple(
                    intField=42,
                    strField="simple",
                    intList=[1, 2, 3],
                    strSet={"hello", "world"},
                    strToIntMap={"one": 1, "two": 2},
                    color=types.Color.NONE,
                    name_="myname",
                ),
                simpleList=[
                    types.Simple(
                        intField=200,
                        strField="face",
                        intList=[4, 5, 6],
                        strSet={"keep", "calm"},
                        strToIntMap={"three": 3, "four": 4},
                        color=types.Color.RED,
                        name_="myname",
                    ),
                    types.Simple(
                        intField=404,
                        strField="b00k",
                        intList=[7, 8, 9],
                        strSet={"carry", "on"},
                        strToIntMap={"five": 5, "six": 6},
                        color=types.Color.GREEN,
                        name_="myname",
                    ),
                ],
                colorToSimpleMap={
                    types.Color.BLUE: types.Simple(
                        intField=500,
                        strField="internal",
                        intList=[10],
                        strSet={"server", "error"},
                        strToIntMap={"seven": 7, "eight": 8, "nine": 9},
                        color=types.Color.BLUE,
                        name_="myname",
                    )
                },
            ),
        )
        self.assertEqual(nested.simpleField.intField, 42)
        self.assertEqual(nested.simpleList[0].intList, [4, 5, 6])
        self.assertEqual(nested.simpleList[1].strSet, {"carry", "on"})
        self.assertEqual(
            nested.colorToSimpleMap[thrift_types.Color.BLUE].color,
            thrift_types.Color.BLUE,
        )

    def test_simple_union(self) -> None:
        simple_union = to_python_struct(thrift_types.Union, types.Union(intField=42))
        self.assertEqual(simple_union.type, thrift_types.Union.Type.intField)
        self.assertEqual(simple_union.value, 42)

    def test_union_with_py3_name_annotation(self) -> None:
        simple_union = to_python_struct(thrift_types.Union, types.Union(name_="myname"))
        self.assertEqual(simple_union.type, thrift_types.Union.Type.name_)
        self.assertEqual(simple_union.value, "myname")

    def test_union_with_containers(self) -> None:
        union_with_list = to_python_struct(
            thrift_types.Union, types.Union(intList=[1, 2, 3])
        )
        self.assertEqual(union_with_list.type, thrift_types.Union.Type.intList)
        self.assertEqual(union_with_list.value, [1, 2, 3])

    def test_complex_union(self) -> None:
        complex_union = to_python_struct(
            thrift_types.Union,
            types.Union(
                simple_=types.Simple(
                    intField=42,
                    strField="simple",
                    intList=[1, 2, 3],
                    strSet={"hello", "world"},
                    strToIntMap={"one": 1, "two": 2},
                    color=types.Color.NONE,
                )
            ),
        )
        self.assertEqual(complex_union.type, thrift_types.Union.Type.simple_)
        self.assertEqual(complex_union.simple_.intField, 42)


class PyLegacytoPythonConverterTest(unittest.TestCase):
    def test_simple(self) -> None:
        simple = to_python_struct(
            thrift_types.Simple,
            ttypes.Simple(
                intField=42,
                strField="simple",
                intList=[1, 2, 3],
                strSet={"hello", "world"},
                strToIntMap={"one": 1, "two": 2},
                color=ttypes.Color.GREEN,
                name="myname",
            ),
        )
        self.assertEqual(simple.intField, 42)
        self.assertEqual(simple.strField, "simple")
        self.assertEqual(simple.intList, [1, 2, 3])
        self.assertEqual(simple.strSet, {"hello", "world"})
        self.assertEqual(simple.strToIntMap, {"one": 1, "two": 2})
        self.assertEqual(simple.color, thrift_types.Color.GREEN)
        self.assertEqual(simple.name_, "myname")

    def test_nested(self) -> None:
        nested = to_python_struct(
            thrift_types.Nested,
            ttypes.Nested(
                simpleField=ttypes.Simple(
                    intField=42,
                    strField="simple",
                    intList=[1, 2, 3],
                    strSet={"hello", "world"},
                    strToIntMap={"one": 1, "two": 2},
                    color=ttypes.Color.NONE,
                    name="myname",
                ),
                simpleList=[
                    ttypes.Simple(
                        intField=200,
                        strField="face",
                        intList=[4, 5, 6],
                        strSet={"keep", "calm"},
                        strToIntMap={"three": 3, "four": 4},
                        color=ttypes.Color.RED,
                        name="myname",
                    ),
                    ttypes.Simple(
                        intField=404,
                        strField="b00k",
                        intList=[7, 8, 9],
                        strSet={"carry", "on"},
                        strToIntMap={"five": 5, "six": 6},
                        color=ttypes.Color.GREEN,
                        name="myname",
                    ),
                ],
                colorToSimpleMap={
                    ttypes.Color.BLUE: ttypes.Simple(
                        intField=500,
                        strField="internal",
                        intList=[10],
                        strSet={"server", "error"},
                        strToIntMap={"seven": 7, "eight": 8, "nine": 9},
                        color=ttypes.Color.BLUE,
                        name="myname",
                    )
                },
            ),
        )
        self.assertEqual(nested.simpleField.intField, 42)
        self.assertEqual(nested.simpleList[0].intList, [4, 5, 6])
        self.assertEqual(nested.simpleList[1].strSet, {"carry", "on"})
        self.assertEqual(
            nested.colorToSimpleMap[thrift_types.Color.BLUE].color,
            thrift_types.Color.BLUE,
        )

    def test_simple_union(self) -> None:
        simple_union = to_python_struct(thrift_types.Union, ttypes.Union(intField=42))
        self.assertEqual(simple_union.type, thrift_types.Union.Type.intField)
        self.assertEqual(simple_union.value, 42)

    def test_union_with_py3_name_annotation(self) -> None:
        simple_union = to_python_struct(thrift_types.Union, ttypes.Union(name="myname"))
        self.assertEqual(simple_union.type, thrift_types.Union.Type.name_)
        self.assertEqual(simple_union.value, "myname")

    def test_union_with_containers(self) -> None:
        union_with_list = to_python_struct(
            thrift_types.Union, ttypes.Union(intList=[1, 2, 3])
        )
        self.assertEqual(union_with_list.type, thrift_types.Union.Type.intList)
        self.assertEqual(union_with_list.value, [1, 2, 3])

    def test_complex_union(self) -> None:
        complex_union = to_python_struct(
            thrift_types.Union,
            ttypes.Union(
                simpleField=ttypes.Simple(
                    intField=42,
                    strField="simple",
                    intList=[1, 2, 3],
                    strSet={"hello", "world"},
                    strToIntMap={"one": 1, "two": 2},
                    color=ttypes.Color.NONE,
                )
            ),
        )
        self.assertEqual(complex_union.type, thrift_types.Union.Type.simple_)
        self.assertEqual(complex_union.simple_.intField, 42)
