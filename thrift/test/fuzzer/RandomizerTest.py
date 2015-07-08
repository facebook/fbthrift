from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import math
import unittest

import six
import six.moves as sm

from thrift import Thrift

import thrift.util.randomizer as randomizer

from fuzz import ttypes

class TestRandomizer(object):
    iterations = 1024

    def get_randomizer(self, ttypes, spec_args, constraints):
        state = randomizer.RandomizerState()
        return state.get_randomizer(ttypes, spec_args, constraints)

class TestBoolRandomizer(unittest.TestCase, TestRandomizer):
    def test_always_true(self):
        cls = self.__class__
        constraints = {'p_true': 1.0}
        gen = self.get_randomizer(Thrift.TType.BOOL, None, constraints)

        for _ in sm.xrange(cls.iterations):
            self.assertTrue(gen.randomize())

    def test_always_false(self):
        cls = self.__class__
        constraints = {'p_true': 0.0}
        gen = self.get_randomizer(Thrift.TType.BOOL, None, constraints)

        for _ in sm.xrange(cls.iterations):
            self.assertFalse(gen.randomize())

class TestEnumRandomizer(unittest.TestCase, TestRandomizer):
    def test_always_valid(self):
        cls = self.__class__
        constraints = {'p_invalid': 0}
        gen = self.get_randomizer(Thrift.TType.I32, ttypes.Color, constraints)

        for _ in sm.xrange(cls.iterations):
            self.assertIn(gen.randomize(), ttypes.Color._VALUES_TO_NAMES)

    def test_never_valid(self):
        cls = self.__class__
        constraints = {'p_invalid': 1}
        gen = self.get_randomizer(Thrift.TType.I32, ttypes.Color, constraints)

        for _ in sm.xrange(cls.iterations):
            self.assertNotIn(gen.randomize(), ttypes.Color._VALUES_TO_NAMES)


class TestIntRandomizer(TestRandomizer):
    def testInRange(self):
        cls = self.__class__
        ttype = cls.ttype
        n_bits = cls.n_bits
        min_ = -(2 ** (n_bits - 1))
        max_ = (2 ** (n_bits - 1)) - 1
        gen = self.get_randomizer(ttype, None, {})
        for _ in sm.xrange(cls.iterations):
            val = gen.randomize()
            self.assertGreaterEqual(val, min_)
            self.assertLessEqual(val, max_)

class TestByteRandomizer(TestIntRandomizer, unittest.TestCase):
    ttype = Thrift.TType.BYTE
    n_bits = 8

class TestI16Randomizer(TestIntRandomizer, unittest.TestCase):
    ttype = Thrift.TType.I16
    n_bits = 16

class TestI32Randomizer(TestIntRandomizer, unittest.TestCase):
    ttype = Thrift.TType.I32
    n_bits = 32

class TestI64Randomizer(TestIntRandomizer, unittest.TestCase):
    ttype = Thrift.TType.I64
    n_bits = 64


class TestFloatRandomizer(TestRandomizer):
    @property
    def randomizer_cls(self):
        return self.__class__.randomizer_cls

    def testZero(self):
        cls = self.__class__
        constraints = {
            'p_zero': 1,
            'p_unreal': 0
        }
        gen = self.get_randomizer(self.randomizer_cls.ttype, None, constraints)
        for _ in sm.xrange(cls.iterations):
            val = gen.randomize()
            self.assertEqual(val, 0.0)

    def testNonZero(self):
        cls = self.__class__
        constraints = {
            'p_zero': 0,
        }
        gen = self.get_randomizer(self.randomizer_cls.ttype, None, constraints)
        for _ in sm.xrange(cls.iterations):
            val = gen.randomize()
            self.assertNotEqual(val, 0.0)

    def testUnreal(self):
        cls = self.__class__
        constraints = {
            'p_unreal': 1
        }
        gen = self.get_randomizer(self.randomizer_cls.ttype, None, constraints)
        for _ in sm.xrange(cls.iterations):
            val = gen.randomize()
            self.assertTrue(
                math.isnan(val) or math.isinf(val)
            )

    def testReal(self):
        cls = self.__class__
        constraints = {
            'p_unreal': 0
        }
        gen = self.get_randomizer(self.randomizer_cls.ttype, None, constraints)
        for _ in sm.xrange(cls.iterations):
            val = gen.randomize()
            self.assertFalse(
                math.isnan(val) or math.isinf(val)
            )

    def testConstant(self):
        cls = self.__class__
        constant = 77.2
        constraints = {
            'mean': constant,
            'std_deviation': 0,
            'p_unreal': 0,
            'p_zero': 0
        }
        gen = self.get_randomizer(self.randomizer_cls.ttype, None, constraints)
        for _ in sm.xrange(cls.iterations):
            val = gen.randomize()
            self.assertEquals(val, constant)

class TestSinglePrecisionRandomizer(TestFloatRandomizer, unittest.TestCase):
    randomizer_cls = randomizer.SinglePrecisionFloatRandomizer
    ttype = Thrift.TType.FLOAT

class TestDoublePrecisionRandomizer(TestFloatRandomizer, unittest.TestCase):
    randomizer_cls = randomizer.DoublePrecisionFloatRandomizer
    ttype = Thrift.TType.DOUBLE

class TestStringRandomizer(TestRandomizer, unittest.TestCase):
    def testInRange(self):
        cls = self.__class__
        ascii_min, ascii_max = randomizer.StringRandomizer.ascii_range

        gen = self.get_randomizer(Thrift.TType.STRING, None, {})
        for _ in sm.xrange(cls.iterations):
            val = gen.randomize()
            for char in val:
                self.assertTrue(ascii_min <= ord(char) <= ascii_max)

    def testEmpty(self):
        cls = self.__class__

        constraints = {'mean_length': 0}
        gen = self.get_randomizer(Thrift.TType.STRING, None, constraints)
        for _ in sm.xrange(cls.iterations):
            val = gen.randomize()
            for char in val:
                self.assertEquals(0, len(val))

class TestListRandomizer(TestRandomizer, unittest.TestCase):
    def testEmpty(self):
        cls = self.__class__

        ttype = Thrift.TType.LIST
        spec_args = (Thrift.TType.I32, None)  # Elements are i32
        constraints = {'mean_length': 0}

        gen = self.get_randomizer(ttype, spec_args, constraints)

        for _ in sm.xrange(cls.iterations):
            val = gen.randomize()
            self.assertEquals(len(val), 0)

    def testElementConstraints(self):
        cls = self.__class__

        ttype = Thrift.TType.LIST
        spec_args = (Thrift.TType.BOOL, None)
        constraints = {'element': {'p_true': 0}}

        gen = self.get_randomizer(ttype, spec_args, constraints)

        for _ in sm.xrange(cls.iterations):
            val = gen.randomize()
            for elem in val:
                self.assertFalse(elem)

class TestSetRandomizer(TestRandomizer, unittest.TestCase):
    def testEmpty(self):
        cls = self.__class__

        ttype = Thrift.TType.SET
        spec_args = (Thrift.TType.I32, None)  # Elements are i32
        constraints = {'mean_length': 0}

        gen = self.get_randomizer(ttype, spec_args, constraints)

        for _ in sm.xrange(cls.iterations):
            val = gen.randomize()
            self.assertEquals(len(val), 0)

    def testElementConstraints(self):
        cls = self.__class__

        ttype = Thrift.TType.SET
        spec_args = (Thrift.TType.BOOL, None)
        constraints = {'element': {'p_true': 0}}

        gen = self.get_randomizer(ttype, spec_args, constraints)

        for _ in sm.xrange(cls.iterations):
            val = gen.randomize()
            for elem in val:
                self.assertFalse(elem)

class TestMapRandomizer(TestRandomizer, unittest.TestCase):
    def testEmpty(self):
        cls = self.__class__

        ttype = Thrift.TType.MAP
        spec_args = (Thrift.TType.I32, None, Thrift.TType.I16, None)
        constraints = {'mean_length': 0}

        gen = self.get_randomizer(ttype, spec_args, constraints)

        for _ in sm.xrange(cls.iterations):
            val = gen.randomize()
            self.assertEquals(len(val), 0)

    def testKeyConstraints(self):
        cls = self.__class__

        ttype = Thrift.TType.MAP
        spec_args = (Thrift.TType.BOOL, None, Thrift.TType.I16, None)
        constraints = {'key': {'p_true': 0}}

        gen = self.get_randomizer(ttype, spec_args, constraints)

        for _ in sm.xrange(cls.iterations):
            val = gen.randomize()
            for elem in val:
                self.assertFalse(elem)

    def testValConstraints(self):
        cls = self.__class__

        ttype = Thrift.TType.MAP
        spec_args = (Thrift.TType.I32, None, Thrift.TType.I32, ttypes.Color)
        constraints = {'value': {'p_invalid': 0}}

        gen = self.get_randomizer(ttype, spec_args, constraints)

        for _ in sm.xrange(cls.iterations):
            val = gen.randomize()
            for elem in six.itervalues(val):
                self.assertIn(elem, ttypes.Color._VALUES_TO_NAMES)


class TestStructRandomizer(TestRandomizer):
    def get_spec_args(self, ttype):
        # (ttype, thrift_spec, is_union)
        return (ttype, ttype.thrift_spec, ttype.isUnion())

    def struct_randomizer(self, ttype=None, constraints={}):
        if ttype is None:
            ttype = self.__class__.ttype
        return self.get_randomizer(
            Thrift.TType.STRUCT,
            self.get_spec_args(ttype),
            constraints
        )

class TestListStructRandomizer(TestStructRandomizer, unittest.TestCase):
    """
    Verify that this struct type is generated correctly

    struct ListStruct {
        1: list<bool> a;
        2: list<i16> b;
        3: list<double> c;
        4: list<string> d;
        5: list<list<i32>> e;
        6: list<map<i32, i32>> f;
        7: list<set<string>> g;
    }
    """

    ttype = ttypes.ListStruct

    def testGeneration(self):
        gen = self.struct_randomizer()
        val = gen.randomize()

        if val.a is not None:
            self.assertIsInstance(val.a, list)
            for elem in val.a:
                self.assertIsInstance(elem, bool)

        if val.b is not None:
            self.assertIsInstance(val.b, list)
            for elem in val.b:
                self.assertIsInstance(elem, int)

        if val.c is not None:
            self.assertIsInstance(val.c, list)
            for elem in val.c:
                self.assertIsInstance(elem, float)

        if val.d is not None:
            self.assertIsInstance(val.d, list)
            for elem in val.d:
                self.assertIsInstance(elem, six.string_types)

        if val.e is not None:
            self.assertIsInstance(val.e, list)
            for elem in val.e:
                self.assertIsInstance(elem, list)
                for sub_elem in elem:
                    self.assertIsInstance(sub_elem, int)

        if val.f is not None:
            self.assertIsInstance(val.f, list)
            for elem in val.f:
                self.assertIsInstance(elem, dict)
                for k, v in six.iteritems(elem):
                    self.assertIsInstance(k, int)
                    self.assertIsInstance(v, int)

        if val.g is not None:
            self.assertIsInstance(val.g, list)
            for elem in val.g:
                self.assertIsInstance(elem, set)
                for sub_elem in elem:
                    self.assertIsInstance(sub_elem, six.string_types)

    def testFieldConstraints(self):
        cls = self.__class__

        constraints = {
            'p_include': 1.0,
            'a': {'element': {'p_true': 1.0}},
            'b': {'mean_length': 0.0},
            'd': {'element': {'mean_length': 0.0}},
            'e': {'element': {'mean_length': 0.0}},
        }

        gen = self.struct_randomizer(constraints=constraints)

        for _ in sm.xrange(cls.iterations):
            val = gen.randomize()
            self.assertIsNotNone(val)
            self.assertIsNotNone(val.a)
            self.assertIsNotNone(val.b)
            self.assertIsNotNone(val.c)
            self.assertIsNotNone(val.d)
            self.assertIsNotNone(val.e)
            self.assertIsNotNone(val.f)
            self.assertIsNotNone(val.g)

            for elem in val.a:
                self.assertTrue(elem)

            self.assertEquals(len(val.b), 0)

            for elem in val.d:
                self.assertEquals(len(elem), 0)

            for elem in val.e:
                self.assertEquals(len(elem), 0)


class TestStructRecursion(TestStructRandomizer, unittest.TestCase):
    ttype = ttypes.BTree

    def max_depth(self, tree):
        """Find the maximum depth of a ttypes.BTree struct"""
        if tree is None:
            return 0

        if isinstance(tree, ttypes.BTreeBranch):
            tree = tree.child

        child_depths = [self.max_depth(child) for child in tree.children]
        if child_depths:
            max_child_depth = max(child_depths)
        else:
            max_child_depth = 0

        return 1 + max_child_depth

    def testDepthZero(self):
        constraints = {'max_recursion_depth': 0}

        gen = self.struct_randomizer(constraints=constraints)

        if not gen.constraints['max_recursion_depth'] == 0:
            raise ValueError('Invalid recursion depth %d' % (
                gen.constraints['max_recursion_depth']))

        val = gen.randomize()

        self.assertEquals(self.max_depth(val), 0)

    def testDepthOne(self):
        cls = self.__class__
        constraints = {'max_recursion_depth': 1}

        gen = self.struct_randomizer(constraints=constraints)

        for _ in sm.xrange(cls.iterations):
            val = gen.randomize()
            self.assertLessEqual(self.max_depth(val), 1)

    def testDepthTwo(self):
        cls = self.__class__
        constraints = {'max_recursion_depth': 2}

        gen = self.struct_randomizer(constraints=constraints)

        for _ in sm.xrange(cls.iterations):
            val = gen.randomize()
            self.assertLessEqual(self.max_depth(val), 2)

if __name__ == '__main__':
    unittest.main()
