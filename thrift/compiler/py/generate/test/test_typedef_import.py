#!/usr/bin/env python2
import unittest
import typedef_test.included.ttypes as included
from typedef_test.includer.ttypes import *

from thrift.Thrift import UnimplementedTypedef


class TypedefUnittest(unittest.TestCase):
    def test_types(self):
        self.assertIs(included.Foo, InclFoo)
        self.assertIs(included.Pod, InclPod)
        self.assertIsInstance(InclPods, UnimplementedTypedef)
        self.assertIs(included.Beep, InclBeep)
        self.assertIs(included.Beep, InclBeepAlso)
        self.assertIsInstance(InclInt64, UnimplementedTypedef)
