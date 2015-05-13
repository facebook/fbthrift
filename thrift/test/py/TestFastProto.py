from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import unittest

import sys
if sys.version_info[0] >= 3:
    from thrift.util.BytesStrIO import BytesStrIO
    StringIO = BytesStrIO
else:
    from cStringIO import StringIO

from thrift.protocol import fastproto, TBinaryProtocol, TCompactProtocol
from thrift.transport.TTransport import TMemoryBuffer

from FastProto.ttypes import AStruct, OneOfEach, TestUnion, StructWithUnion, \
        NegativeFieldId

class ReadOnlyBufferWithRefill(TMemoryBuffer):
    def __init__(self, index, value=None):
        self._value = value
        self._index = index
        self.refill_called = 0
        if value is None:
            self._readBuffer = StringIO(b"")
        elif index < 0 or index > len(value):
            raise RuntimeError("index overflow")
        else:
            self._readBuffer = StringIO(value[:index])

    def cstringio_refill(self, partialread, reqlen):
        self.refill_called += 1
        self._readBuffer = StringIO(partialread + self._value[self._index:])
        return self._readBuffer

class AbstractTest():
    def encode_helper(self, obj):
        buf = fastproto.encode(obj, [obj.__class__, obj.thrift_spec,
            obj.isUnion()], utf8strings=0, protoid=self.PROTO)

        trans = TMemoryBuffer(buf)
        if self.PROTO == 0:
            proto = TBinaryProtocol.TBinaryProtocol(trans)
        else:
            proto = TCompactProtocol.TCompactProtocol(trans)

        obj_new = obj.__class__()
        obj_new.read(proto)
        self.assertEqual(obj, obj_new)

    def decode_helper(self, obj, split=1.0):
        trans = TMemoryBuffer()
        if self.PROTO == 0:
            proto = TBinaryProtocol.TBinaryProtocol(trans)
        else:
            proto = TCompactProtocol.TCompactProtocol(trans)

        obj.write(proto)
        index = int(split * len(trans.getvalue()))
        trans = ReadOnlyBufferWithRefill(index, trans.getvalue())
        obj_new = obj.__class__()
        fastproto.decode(obj_new, trans, [obj.__class__, obj.thrift_spec,
            obj.isUnion()], utf8strings=0, protoid=self.PROTO)
        self.assertEqual(obj, obj_new)
        # Verify the entire buffer is read
        self.assertEqual(len(trans._readBuffer.read()), 0)
        if split != 1.0:
            self.assertEqual(1, trans.refill_called)

    def buildOneOfEach(self):
        return OneOfEach(
                aBool=True,
                aByte=1,
                anInteger16=234,
                anInteger32=12345,
                anInteger64=12345678910,
                aString=u'\x00hello',
                aBinary=b'\x00\x01\x00',
                aDouble=1234567.901,
                aFloat=12345.0,
                aList=[12, 34, 567, 89],
                aSet=set(["hello", "world", "good", "bye"]),
                aMap={"hello": 1, "world": 20},
                aStruct=AStruct(aString="str", anInteger=109))

    def buildOneOfEachB(self):
        return OneOfEach(
                aBool=True,
                aByte=1,
                anInteger16=234,
                anInteger32=12345,
                anInteger64=12345678910,
                aString=u'\x00hello',
                aBinary=b'\x00\x01\x00',
                aDouble=1234567.901,
                aFloat=12345.0,
                aList=[12, 34, 567, 89],
                aSet=set([b"hello", b"world", b"good", b"bye"]),
                aMap={b"hello": 1, b"world": 20},
                aStruct=AStruct(aString=b"str", anInteger=109))

    def test_encode(self):
        self.encode_helper(self.buildOneOfEach())

    def test_encode_union(self):
        u = TestUnion(i32_field=12)
        self.encode_helper(u)
        u.set_string_field("hello")
        self.encode_helper(u)
        u.set_struct_field(AStruct(aString="hello"))
        self.encode_helper(u)
        swu = StructWithUnion(aUnion=u, aString="world")
        self.encode_helper(swu)

    def test_decode(self):
        self.decode_helper(self.buildOneOfEachB())
        # Test when ensureMapBegin needs to verify the buffer has
        # at least a varint and 1 more byte.
        self.decode_helper(OneOfEach(aMap={b"h": 1}), split=0.1)

    def test_decode_union(self):
        u = TestUnion(i32_field=123)
        self.decode_helper(u)
        u.set_string_field(b"world")
        self.decode_helper(u)
        u.set_struct_field(AStruct(aString=b"world"))
        self.decode_helper(u)
        swu = StructWithUnion(aUnion=u, aString=b"world")
        self.decode_helper(swu)

    def test_refill(self):
        for idx in range(1, 10):
            self.decode_helper(self.buildOneOfEachB(), split=0.1*idx)

    def test_negative_fid(self):
        self.encode_helper(NegativeFieldId(anInteger=20,
            aString='hello', aDouble=1.2))
        self.decode_helper(NegativeFieldId(anInteger=344444,
            aString=b'hello again', aDouble=1.34566))

    def test_empty_container(self):
        self.encode_helper(OneOfEach(aSet=set(), aList=[], aMap={}))
        self.decode_helper(OneOfEach(aSet=set(), aList=[], aMap={}))

class FastBinaryTest(AbstractTest, unittest.TestCase):
    PROTO = 0

class FastCompactTest(AbstractTest, unittest.TestCase):
    PROTO = 2

if __name__ == "__main__":
    unittest.main()
