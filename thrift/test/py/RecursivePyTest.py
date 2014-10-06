from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import unittest

from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.protocol.TBinaryProtocol import TBinaryProtocolFactory, \
        TBinaryProtocolAcceleratedFactory
from thrift.util.Serializer import serialize, deserialize
from Recursive.ttypes import *

class AbstractTestRecursivePythonStructs():
    def test_tree(self):
        tree = RecTree()
        child = RecTree()
        tree.children = [child]
        ser = serialize(self.fac, tree)
        result = RecTree()
        result = deserialize(self.fac, ser, result)
        self.assertEqual(result, tree)

    def test_list(self):
        l = RecList()
        l2 = RecList()
        l.next = l2
        ser = serialize(self.fac, l)
        result = RecList()
        result = deserialize(self.fac, ser, result)
        self.assertIsNotNone(result.next)
        self.assertIsNone(result.next.next)

    def test_corec(self):
        c = CoRec()
        r = CoRec2()
        c.other = r
        ser = serialize(self.fac, c)
        result = CoRec()
        result = deserialize(self.fac, ser, result)
        self.assertIsNotNone(c.other)
        self.assertIsNone(c.other.other)

class TestBinary(AbstractTestRecursivePythonStructs, unittest.TestCase):
    def setUp(self):
        self.fac = TBinaryProtocolFactory()

class TestBinaryAccelerated(AbstractTestRecursivePythonStructs,
        unittest.TestCase):
    def setUp(self):
        self.fac = TBinaryProtocolAcceleratedFactory()

if __name__ == '__main__':
    unittest.main()
