from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

# @lint-avoid-pyflakes2
# @lint-avoid-pyflakes3

import unittest
import math

from thrift.protocol import TSimpleJSONProtocol
from thrift.transport.TTransport import TMemoryBuffer
from thrift.util import Serializer

from SimpleJSONRead.ttypes import SomeStruct, Stuff

def writeToJSON(obj):
    trans = TMemoryBuffer()
    proto = TSimpleJSONProtocol.TSimpleJSONProtocol(trans)
    obj.write(proto)
    return trans.getvalue()

def readStuffFromJSON(jstr, struct_type=Stuff):
    stuff = struct_type()
    trans = TMemoryBuffer(jstr)
    proto = TSimpleJSONProtocol.TSimpleJSONProtocol(trans,
                                                    struct_type.thrift_spec)
    stuff.read(proto)
    return stuff

class TestSimpleJSONRead(unittest.TestCase):
    def test_primitive_type(self):
        stuff = Stuff(
                aString="hello",
                aShort=10,
                anInteger=23990,
                aLong=123456789012,
                aDouble=1234567.9,
                aBool=True)
        j = writeToJSON(stuff)
        stuff_read = readStuffFromJSON(j)
        self.assertEqual(stuff_read.aString, "hello")
        self.assertEqual(stuff_read.aShort, 10)
        self.assertEqual(stuff_read.anInteger, 23990)
        self.assertEqual(stuff_read.aLong, 123456789012)
        self.assertEqual(stuff_read.aDouble, 1234567.9)
        self.assertTrue(stuff_read.aBool)

    def test_escape_string(self):
        stuff = Stuff(
            aString=b'\\hello')
        j = writeToJSON(stuff)
        stuff_read = readStuffFromJSON(j)
        self.assertEqual(stuff_read.aString, '\\hello')

    def test_unusual_numbers(self):
        j = '{ "aListOfDouble": ["inf", "-inf", "nan"]}'
        stuff_read = readStuffFromJSON(j)
        self.assertEqual(len(stuff_read.aListOfDouble), 3)
        self.assertTrue(math.isinf(stuff_read.aListOfDouble[0]))
        self.assertTrue(math.isinf(stuff_read.aListOfDouble[1]))
        self.assertTrue(math.isnan(stuff_read.aListOfDouble[2]))

    def test_unexpected_field(self):
        ss = SomeStruct(anInteger=1)
        j = '{ "anInteger": 101, "unexpected": 111.1}'
        struct_read = readStuffFromJSON(j, struct_type=SomeStruct)
        self.assertEqual(struct_read.anInteger, 101)

    def test_map(self):
        stuff = Stuff(
                aMap={1: {"hello": [1,2,3,4],
                          "world": [5,6,7,8]},
                      2: {"good": [100, 200],
                          "bye": [300, 400]}
                      },
                anotherString="Hey")
        j = writeToJSON(stuff)
        stuff_read = readStuffFromJSON(j)
        self.assertEqual(len(stuff_read.aMap), 2)
        self.assertEqual(stuff_read.aMap[1]["hello"], [1,2,3,4])
        self.assertEqual(stuff_read.aMap[1]["world"], [5,6,7,8])
        self.assertEqual(stuff_read.aMap[2]["good"], [100, 200])
        self.assertEqual(stuff_read.aMap[2]["bye"], [300, 400])
        self.assertEqual(stuff_read.anotherString, "Hey")

    def test_list(self):
        stuff = Stuff(
                aList=[
                    [[["hello", "world"], ["good", "bye"]]],
                    [[["what", "is"], ["going", "on"]]]],
                anotherString="Hey")
        j = writeToJSON(stuff)
        stuff_read = readStuffFromJSON(j)
        self.assertEqual(len(stuff_read.aList), 2)
        self.assertEqual(stuff_read.aList[0][0][0], ["hello", "world"])
        self.assertEqual(stuff_read.aList[0][0][1], ["good", "bye"])
        self.assertEqual(stuff_read.aList[1][0][0], ["what", "is"])
        self.assertEqual(stuff_read.aList[1][0][1], ["going", "on"])
        self.assertEqual(stuff_read.anotherString, "Hey")

    def test_set(self):
        stuff = Stuff(
                aListOfSet=[set(["hello"]), set(["world"])],
                anotherString="Hey")
        j = writeToJSON(stuff)
        stuff_read = readStuffFromJSON(j)
        self.assertEqual(len(stuff_read.aListOfSet), 2)
        self.assertEqual(stuff_read.aListOfSet[0], set(["hello"]))
        self.assertEqual(stuff_read.aListOfSet[1], set(["world"]))
        self.assertEqual(stuff_read.anotherString, "Hey")

    def test_struct(self):
        stuff = Stuff(
                aStruct=SomeStruct(anInteger=12,
                                   aMap={"hi": 1.5}),
                aListOfStruct=[
                    SomeStruct(anInteger=10,
                               aMap={"good": 2.0}),
                    SomeStruct(anInteger=11,
                               aMap={"bye": 1.0})],
                anotherString="Hey"
            )
        j = writeToJSON(stuff)
        stuff_read = readStuffFromJSON(j)
        self.assertEqual(len(stuff_read.aListOfStruct), 2)
        self.assertEqual(stuff_read.aListOfStruct[0].anInteger, 10)
        self.assertEqual(stuff_read.aListOfStruct[0].aMap["good"], 2.0)
        self.assertEqual(stuff_read.aListOfStruct[1].anInteger, 11)
        self.assertEqual(stuff_read.aListOfStruct[1].aMap["bye"], 1.0)
        self.assertEqual(stuff_read.anotherString, "Hey")

    def test_deserializer(self):
        j = '{"aShort": 1, "anInteger": 2, "aLong": 3}'
        stuff = Stuff()
        Serializer.deserialize(
            TSimpleJSONProtocol.TSimpleJSONProtocolFactory(), j, stuff)
        self.assertEqual(stuff.aShort, 1)
        self.assertEqual(stuff.anInteger, 2)
        self.assertEqual(stuff.aLong, 3)

if __name__ == '__main__':
    unittest.main()
