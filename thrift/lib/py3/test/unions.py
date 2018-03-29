#!/usr/bin/env python3
import unittest

from testing.types import (
    Integers, ComplexUnion, easy, Color, ReservedUnion
)
from thrift.py3.types import Union
from thrift.py3 import deserialize, Protocol


class UnionTests(unittest.TestCase):

    def test_hashability(self) -> None:
        hash(Integers())

    def test_deserialize_empty(self) -> None:
        x = deserialize(Integers, b'{}', Protocol.JSON)
        self.assertEqual(x.type, Integers.Type.EMPTY)

    def test_union_usage(self) -> None:
        value = hash('i64')
        x = Integers(large=value)
        self.assertIsInstance(x, Union)
        self.assertEqual(x.type, x.get_type())
        self.assertEqual(x.type, Integers.Type.large)
        self.assertEqual(x.value, value)
        # Hashing Works
        s = set([x])
        self.assertIn(x, s)
        # Repr is useful
        rx = repr(x)
        self.assertIn(str(value), rx)
        self.assertIn(x.type.name, rx)

    def test_multiple_values(self) -> None:
        with self.assertRaises(TypeError):
            Integers(small=1, large=2)

    def test_wrong_type(self) -> None:
        x = Integers(small=1)
        with self.assertRaises(TypeError):
            x.large
        x.small

    def test_integers_fromValue(self) -> None:
        tiny = 2 ** 7 - 1
        small = 2 ** 15 - 1
        medium = 2 ** 31 - 1
        large = 2 ** 63 - 1
        union = Integers.fromValue(tiny)
        self.assertEqual(union.type, Integers.Type.tiny)
        union = Integers.fromValue(small)
        self.assertEqual(union.type, Integers.Type.small)
        union = Integers.fromValue(medium)
        self.assertEqual(union.type, Integers.Type.medium)
        union = Integers.fromValue(large)
        self.assertEqual(union.type, Integers.Type.large)

    def test_complexunion_fromValue(self) -> None:
        tiny = 2 ** 7 - 1
        large = 2 ** 63 - 1
        afloat = 3.141592025756836  # Hand crafted to be representable as float
        adouble = 3.14159265358
        union = ComplexUnion.fromValue(tiny)
        self.assertEqual(union.type, ComplexUnion.Type.tiny)
        union = ComplexUnion.fromValue(large)
        self.assertEqual(union.type, ComplexUnion.Type.large)
        union = ComplexUnion.fromValue(afloat)
        self.assertEqual(union.value, afloat)
        self.assertEqual(union.type, ComplexUnion.Type.float_val)
        union = ComplexUnion.fromValue(adouble)
        self.assertEqual(union.value, adouble)
        self.assertEqual(union.type, ComplexUnion.Type.double_val)
        union = ComplexUnion.fromValue(Color.red)
        self.assertEqual(union.type, ComplexUnion.Type.color)
        union = ComplexUnion.fromValue(easy())
        self.assertEqual(union.type, ComplexUnion.Type.easy_struct)
        union = ComplexUnion.fromValue("foo")
        self.assertEqual(union.type, ComplexUnion.Type.text)
        union = ComplexUnion.fromValue(b"ar")
        self.assertEqual(union.type, ComplexUnion.Type.raw)
        union = ComplexUnion.fromValue(True)
        self.assertEqual(union.type, ComplexUnion.Type.truthy)

    def test_reserved_union(self) -> None:
        x = ReservedUnion(from_="foo")
        self.assertIsInstance(x, Union)
        self.assertEqual(x.type, ReservedUnion.Type.from_)
        self.assertEqual(x.value, "foo")
        self.assertEqual(x.from_, "foo")

        x = ReservedUnion(nonlocal_=3)
        self.assertIsInstance(x, Union)
        self.assertEqual(x.type, ReservedUnion.Type.nonlocal_)
        self.assertEqual(x.value, 3)
        self.assertEqual(x.nonlocal_, 3)

        x = ReservedUnion(ok="bar")
        self.assertIsInstance(x, Union)
        self.assertEqual(x.type, ReservedUnion.Type.ok)
        self.assertEqual(x.value, "bar")
        self.assertEqual(x.ok, "bar")
