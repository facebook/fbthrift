#!/usr/bin/env python

#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#

import glob
import math
import json
import sys
sys.path.insert(0, './gen-py')
lib_path = glob.glob('../../lib/py/build/lib.*')
if lib_path:
    sys.path.insert(0, lib_path[0])

from myBinaryStruct.ttypes import *
from myBoolStruct.ttypes import *
from myByteStruct.ttypes import *
from myComplexStruct.ttypes import *
from myDoubleStruct.ttypes import *
from myI16Struct.ttypes import *
from myI32Struct.ttypes import *
from myMixedStruct.ttypes import *
from mySetStruct.ttypes import *
from myMapStruct.ttypes import *
from myNestedMapStruct.ttypes import *
from mySimpleStruct.ttypes import *
from myStringStruct.ttypes import *
from myDoubleListStruct.ttypes import *
from myCollectionStruct.ttypes import *

from thrift.protocol.TSimpleJSONProtocol import TSimpleJSONProtocolFactory
from thrift.protocol.TProtocol import TProtocolException

import unittest
import time

class SimpleJSONToThriftTest(unittest.TestCase):

    def setUp(self):
        self.binaryStruct = myBinaryStruct(a='xyzzy')

        self.boolStruct1 = myBoolStruct(a=True)
        self.boolStruct2 = myBoolStruct(a=False)

        self.byteStruct = myByteStruct(a=101)
        self.byteStructBad = myByteStruct(a=3232)

        self.complexStruct1 = myComplexStruct(
               a=mySimpleStruct(
                   a=True,
                   b=92,
                   c=902,
                   d=65536,
                   e=123456789,
                   f=3.1415,
                   g='Whan that Aprille'
               ),
               b=[314, 15, 9, 26535],
               c={"qwerty": mySimpleStruct(c=1),
                  "slippy": mySimpleStruct(a=False, b=-4, c=5)},
               e=EnumTest.EnumTwo,
               x=ExceptionTest("test")
        )
        self.complexStruct2 = myComplexStruct()

        self.doubleStruct1 = myDoubleStruct(a=-2.192)
        self.doubleStruct2 = myDoubleStruct(a=float('inf'))
        self.doubleStruct3 = myDoubleStruct(a=float('-inf'))

        self.I16Struct = myI16Struct(a=4567)
        self.I16StructBad = myI16Struct(a=0xFEDCBA987)

        self.I32Struct = myI32Struct(a=12131415)
        self.I32StructBad = myI32Struct(a=0xFFFFFFFFEDCBA)

        self.mixedStruct = myMixedStruct(
               a=[],
               b=[mySuperSimpleStruct(a=5)],
               c={'flame': -8, 'fire': -191},
               d={},
               e=set([1, 2, 3, 4]))

        self.setStruct1 = mySetStruct(a=set([4, 8, 15, 16]))
        self.setStruct2 = mySetStruct(a=set([]))
        self.setStructBad = mySetStruct(a=set([1, 0xFFFFFFFFFF, 2]))

        self.mapStruct = myMapStruct(
            stringMap={"a": "A", "b": "B"},
            boolMap={True: "True", False: "False"},
            byteMap={1: "one", 2: "two"},
            doubleMap={float("0.1"): "0.one", float("0.2"): "0.two"},
            enumMap={1: "male", 2: "female"})

        self.nestedMapStruct = myNestedMapStruct(
            maps={"1": {"1": mySimpleStruct(c=1)},
                  "2": {"2": mySimpleStruct(a=False, c=2)}})

        self.simpleStruct1 = mySimpleStruct(
               a=False,
               b=87,
               c=7880,
               d=-7880,
               e=-1,
               f=-0.1,
               g='T-bone')
        self.simpleStruct2 = mySimpleStruct(c=9)
        self.simpleStructBad = mySimpleStruct()

        self.stringStruct1 = myStringStruct(a='')
        self.stringStruct2 = myStringStruct()
        self.stringStruct3 = myStringStruct(a="foobar")

        self.collectionStruct1 = myCollectionStruct(
            l=[1.11, 2.22],
            s=set([1.11, 2.22]),
            m={1.11: 1.11, 2.22: 2.22},
            ll=[myTestStruct(d=3.33)],
        )
        self.collectionStruct2 = myCollectionStruct(
            l=[float("nan")],
            s=set([float("nan")]),
            m={float("nan"): float("nan")},
            ll=[myTestStruct(d=float("nan"))],
        )

    def _serialize(self, obj):
        trans = TTransport.TMemoryBuffer()
        prot = TSimpleJSONProtocolFactory().getProtocol(trans)
        obj.write(prot)
        return trans.getvalue()

    def _testStruct(self, struct, is_empty=False):
        gen = struct.__class__()

        if not is_empty:
            self.assertNotEqual(gen, struct)
        else:
            self.assertEqual(gen, struct)

        gen.readFromJson(self._serialize(struct))
        self.assertEqual(gen, struct)

    def _testBadStruct(self, struct, is_empty=False):
        try:
            self._testStruct(struct, is_empty)
            self.fail()
        except TProtocolException as e:
            pass

    def testBinaryStruct(self):
        self._testStruct(self.binaryStruct)

    def testBoolStruct(self):
        self._testStruct(self.boolStruct1)
        self._testStruct(self.boolStruct2)

    def testByteStruct(self):
        self._testStruct(self.byteStruct)
        self._testBadStruct(self.byteStructBad)

    def testComplexStruct(self):
        self._testStruct(self.complexStruct1)
        self._testStruct(self.complexStruct2, True)

    def testDoubleStruct(self):
        self._testStruct(self.doubleStruct1)
        self._testStruct(self.doubleStruct2)
        self._testStruct(self.doubleStruct3)

    def testI16Struct(self):
        self._testStruct(self.I16Struct)
        self._testBadStruct(self.I16StructBad)

    def testI32Struct(self):
        self._testStruct(self.I32Struct)
        self._testBadStruct(self.I32StructBad)

    def testMixedStruct(self):
        self._testStruct(self.mixedStruct)

    def testSetStruct(self):
        self._testStruct(self.setStruct1)
        self._testStruct(self.setStruct2)
        self._testBadStruct(self.setStructBad)

    def testMapStruct(self):
        self._testStruct(self.mapStruct)

    def testNestedMapStruct(self):
        self._testStruct(self.nestedMapStruct)

    def testSimpleStruct(self):
        self._testStruct(self.simpleStruct1)
        self._testStruct(self.simpleStruct2)
        self._testBadStruct(self.simpleStructBad, True)

    def testStringStruct(self):
        self._testStruct(self.stringStruct1)
        self._testStruct(self.stringStruct2, True)
        self._testStruct(self.stringStruct3)

    def testCollectionStruct(self):
        self._testStruct(self.collectionStruct1)

        # Have to test manually because nan != nan
        gen = self.collectionStruct2.__class__()
        gen.readFromJson(self._serialize(self.collectionStruct2))
        self.assertTrue(math.isnan(gen.l[0]))
        self.assertTrue(math.isnan(list(gen.s)[0]))
        self.assertTrue(math.isnan(gen.m.values()[0]))
        self.assertTrue(math.isnan(gen.ll[0].d))

def suite():
    suite = unittest.TestSuite()
    loader = unittest.TestLoader()

    suite.addTest(loader.loadTestsFromTestCase(SimpleJSONToThriftTest))
    return suite

if __name__ == "__main__":
    unittest.main(defaultTest="suite",
            testRunner=unittest.TextTestRunner(verbosity=2))
