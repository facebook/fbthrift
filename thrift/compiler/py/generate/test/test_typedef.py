#!/usr/bin/env python2
import unittest
import typedef_test.included.ttypes as included
import typedef_test.includer.ttypes as includer

from thrift.Thrift import UnimplementedTypedef


class TypedefUnittest(unittest.TestCase):
    def test_types(self):
        self.assertIs(included.Foo, includer.InclFoo)
        self.assertIs(included.Pod, includer.InclPod)
        self.assertIsInstance(includer.InclPods, UnimplementedTypedef)
        self.assertIs(included.Beep, includer.InclBeep)
        self.assertIs(included.Beep, included.BeepAlso)
        self.assertIs(included.Beep, includer.InclBeepAlso)
        self.assertIsInstance(includer.InclInt64, UnimplementedTypedef)
