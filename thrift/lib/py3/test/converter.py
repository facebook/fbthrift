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
from thrift.py3.converter import to_py3_struct


class ConverterTest(unittest.TestCase):
    def test_simple(self) -> None:
        simple = to_py3_struct(
            types.Simple,
            ttypes.Simple(
                intField=42,
                strField="simple",
                intList=[1, 2, 3],
                strSet={"hello", "world"},
                strToIntMap={"one": 1, "two": 2},
                color=ttypes.Color.GREEN,
            ),
        )
        self.assertEqual(simple.intField, 42)
        self.assertEqual(simple.strField, "simple")
        self.assertEqual(simple.intList, [1, 2, 3])
        self.assertEqual(simple.strSet, {"hello", "world"})
        self.assertEqual(simple.strToIntMap, {"one": 1, "two": 2})
        self.assertEqual(simple.color, types.Color.GREEN)

    def test_nested(self) -> None:
        nested = to_py3_struct(
            types.Nested,
            ttypes.Nested(
                simpleField=ttypes.Simple(
                    intField=42,
                    strField="simple",
                    intList=[1, 2, 3],
                    strSet={"hello", "world"},
                    strToIntMap={"one": 1, "two": 2},
                    color=ttypes.Color.NONE,
                ),
                simpleList=[
                    ttypes.Simple(
                        intField=200,
                        strField="face",
                        intList=[4, 5, 6],
                        strSet={"keep", "calm"},
                        strToIntMap={"three": 3, "four": 4},
                        color=ttypes.Color.RED,
                    ),
                    ttypes.Simple(
                        intField=404,
                        strField="b00k",
                        intList=[7, 8, 9],
                        strSet={"carry", "on"},
                        strToIntMap={"five": 5, "six": 6},
                        color=ttypes.Color.GREEN,
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
                    )
                },
            ),
        )
        self.assertEqual(nested.simpleField.intField, 42)
        self.assertEqual(nested.simpleList[0].intList, [4, 5, 6])
        self.assertEqual(nested.simpleList[1].strSet, {"carry", "on"})
        self.assertEqual(
            nested.colorToSimpleMap[types.Color.BLUE].color, types.Color.BLUE
        )

    def test_simple_union(self) -> None:
        simple_union = to_py3_struct(types.Union, ttypes.Union(intField=42))
        self.assertEqual(simple_union.type, types.Union.Type.intField)
        self.assertEqual(simple_union.value, 42)

    def test_union_with_containers(self) -> None:
        union_with_list = to_py3_struct(types.Union, ttypes.Union(intList=[1, 2, 3]))
        self.assertEqual(union_with_list.type, types.Union.Type.intList)
        self.assertEqual(union_with_list.value, [1, 2, 3])

    def test_complex_union(self) -> None:
        complex_union = to_py3_struct(
            types.Union,
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
        self.assertEqual(complex_union.type, types.Union.Type.simpleField)
        self.assertEqual(complex_union.simpleField.intField, 42)
