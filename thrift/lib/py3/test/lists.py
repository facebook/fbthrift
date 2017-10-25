#!/usr/bin/env python3
import unittest

from testing.types import int_list


class ListTests(unittest.TestCase):
    def test_negative_indexes(self):
        length = len(int_list)
        for i in range(length):
            self.assertEquals(int_list[i], int_list[i - length])

        with self.assertRaises(IndexError):
            int_list[-length - 1]
