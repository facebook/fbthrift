#!/usr/bin/env python3
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

import unittest

import convertible.ttypes as ttypes
import convertible.types as types
from thrift.util.converter import to_py_struct


class ConverterTest(unittest.TestCase):
    def test_simple(self) -> None:
        simple = to_py_struct(
            ttypes.Simple,
            types.Simple(
                intField=42,
                strField="simple",
                intList=[1, 2, 3],
                strSet={"hello", "world"},
                strToIntMap={"one": 1, "two": 2},
                color=types.Color.GREEN,
            ),
        )
        self.assertEqual(simple.intField, 42)
        self.assertEqual(simple.strField, "simple")
        self.assertEqual(simple.intList, [1, 2, 3])
        self.assertEqual(simple.strSet, {"hello", "world"})
        self.assertEqual(simple.strToIntMap, {"one": 1, "two": 2})
        self.assertEqual(simple.color, ttypes.Color.GREEN)

    def test_nested(self) -> None:
        nested = to_py_struct(
            ttypes.Nested,
            types.Nested(
                simpleField=types.Simple(
                    intField=42,
                    strField="simple",
                    intList=[1, 2, 3],
                    strSet={"hello", "world"},
                    strToIntMap={"one": 1, "two": 2},
                    color=types.Color.NONE,
                ),
                simpleList=[
                    types.Simple(
                        intField=200,
                        strField="face",
                        intList=[4, 5, 6],
                        strSet={"keep", "calm"},
                        strToIntMap={"three": 3, "four": 4},
                        color=types.Color.RED,
                    ),
                    types.Simple(
                        intField=404,
                        strField="b00k",
                        intList=[7, 8, 9],
                        strSet={"carry", "on"},
                        strToIntMap={"five": 5, "six": 6},
                        color=types.Color.GREEN,
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
                    )
                },
            ),
        )
        self.assertEqual(nested.simpleField.intField, 42)
        self.assertEqual(nested.simpleList[0].intList, [4, 5, 6])
        self.assertEqual(nested.simpleList[1].strSet, {"carry", "on"})
        self.assertEqual(
            nested.colorToSimpleMap[ttypes.Color.BLUE].color, ttypes.Color.BLUE
        )

    def test_simple_union(self) -> None:
        simple_union = to_py_struct(ttypes.Union, types.Union(intField=42))
        self.assertEqual(simple_union.get_intField(), 42)

    def test_union_with_containers(self) -> None:
        union_with_list = to_py_struct(ttypes.Union, types.Union(intList=[1, 2, 3]))
        self.assertEqual(union_with_list.get_intList(), [1, 2, 3])

    def test_complex_union(self) -> None:
        complex_union = to_py_struct(
            ttypes.Union,
            types.Union(
                simpleField=types.Simple(
                    intField=42,
                    strField="simple",
                    intList=[1, 2, 3],
                    strSet={"hello", "world"},
                    strToIntMap={"one": 1, "two": 2},
                    color=types.Color.NONE,
                )
            ),
        )
        self.assertEqual(complex_union.get_simpleField().intField, 42)
