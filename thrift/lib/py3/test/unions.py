#!/usr/bin/env python3
import unittest

from testing.types import Integers, IntegersType
from thrift.py3.types import Union


class UnionTests(unittest.TestCase):
    def test_union_usage(self):
        value = hash('i64')
        x = Integers(large=value)
        self.assertIsInstance(x, Union)
        self.assertEqual(x.type, x.get_type())
        self.assertEqual(x.type, IntegersType.large)
        self.assertEqual(x.value, value)
        # Hashing Works
        s = set([x])
        self.assertIn(x, s)
        # Repr is useful
        rx = repr(x)
        self.assertIn(str(value), rx)
        self.assertIn(x.type.name, rx)

    def test_multiple_values(self):
        with self.assertRaises(TypeError):
            Integers(small=1, large=2)

    def test_wrong_type(self):
        x = Integers(small=1)
        with self.assertRaises(TypeError):
            x.large
        x.small
