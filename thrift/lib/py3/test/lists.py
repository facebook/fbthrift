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

import itertools
import unittest

from testing.types import I32List, StringList, StrList2D, easy, int_list
from thrift.py3.types import Container


class ListTests(unittest.TestCase):
    def test_negative_indexes(self) -> None:
        length = len(int_list)
        for i in range(length):
            self.assertEquals(int_list[i], int_list[i - length])

        with self.assertRaises(IndexError):
            int_list[-length - 1]

    def test_list_of_None(self) -> None:
        with self.assertRaises(TypeError):
            I32List([None, None, None])  # type: ignore

    def test_list_creation_with_list_items(self) -> None:
        a = ["one", "two", "three"]
        b = ["cyan", "magenta", "yellow"]
        c = ["foo", "bar", "baz"]
        d = ["I", "II", "III"]
        StrList2D([a, b, c, d])
        with self.assertRaises(TypeError):
            StrList2D([a, [None]])  # type: ignore

    def test_list_add(self) -> None:
        other_list = [99, 88, 77, 66, 55]
        new_list = int_list + other_list
        self.assertIsInstance(new_list, type(int_list))
        # Insure the items from both lists are in the new_list
        self.assertEquals(new_list, list(itertools.chain(int_list, other_list)))

    def test_list_radd(self) -> None:
        other_list = [99, 88, 77, 66, 55]
        new_list = other_list + int_list
        self.assertIsInstance(new_list, list)
        self.assertEquals(new_list, list(itertools.chain(other_list, int_list)))
        new_list = tuple(other_list) + int_list  # type: ignore
        self.assertIsInstance(new_list, tuple)

    def test_list_creation(self) -> None:
        I32List(range(10))
        with self.assertRaises(TypeError):
            I32List([1, "b", "c", "four"])  # type: ignore

    def test_hashability(self) -> None:
        hash(easy().val_list)
        hash(I32List(range(10)))

    def test_index(self) -> None:
        x = I32List([1, 2, 3, 4, 1, 2, 3, 4])
        y = list(x)
        self.assertEqual(x.index(2), y.index(2))
        self.assertEqual(x.index(2, 3), y.index(2, 3))
        self.assertEqual(x.index(2, 0, 2), y.index(2, 0, 2))
        with self.assertRaises(ValueError):
            raise Exception(x.index(4, 0, 2))

        with self.assertRaises(ValueError):
            y.index(4, 0, 2)
        self.assertEqual(x.index(4, -20, -2), y.index(4, -20, -2))

    def test_splicing(self) -> None:
        x = I32List([1, 2, 3, 4, 1, 2, 3, 4])
        y = list(x)
        self.assertEqual(x[2:], y[2:])
        self.assertEqual(x[:5], y[:5])
        self.assertEqual(x[:0], y[:0])
        self.assertEqual(x[-5:-1], y[-5:-1])

    def test_comparisons(self) -> None:
        x = I32List([1, 2, 3, 4])
        y = I32List([1, 2, 3, 4, 5])
        z = I32List([1, 2, 3, 1, 10])

        self.assertLess(x, y)
        self.assertLess(z, x)
        self.assertLess(z, y)
        self.assertNotEqual(z, y)
        self.assertNotEqual(x, y)
        self.assertNotEqual(z, x)
        self.assertGreater(y, x)
        self.assertGreater(x, z)
        self.assertGreaterEqual(x, z)
        self.assertLessEqual(x, y)

        x2 = I32List([1, 2, 3, 4])
        self.assertEqual(x, x2)
        self.assertLessEqual(x, x2)
        self.assertGreaterEqual(x, x2)

    def test_is_container(self) -> None:
        self.assertIsInstance(int_list, Container)
        self.assertIsInstance(I32List([1, 2, 3]), Container)
        self.assertIsInstance(StrList2D([["a", "b"], ["c", "d"]]), Container)

    def test_string_list(self) -> None:
        StringList(["hello", "world"])
        with self.assertRaises(TypeError):
            StringList("hello")
