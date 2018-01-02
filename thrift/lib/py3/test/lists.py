#!/usr/bin/env python3
import unittest
import itertools

from testing.types import int_list, I32List, StrList2D, easy


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
        a = ['one', 'two', 'three']
        b = ['cyan', 'magenta', 'yellow']
        c = ['foo', 'bar', 'baz']
        d = ['I', 'II', 'III']
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
            I32List([1, 'b', 'c', 'four'])  # type: ignore

    def test_hashability(self) -> None:
        hash(easy().val_list)
        hash(I32List(range(10)))
