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

from thrift.protocol import fastproto, TBinaryProtocol, TCompactProtocol, \
    TProtocol
from thrift.transport.TTransport import TMemoryBuffer

from FastProto.ttypes import AStruct, OneOfEach, TestUnion, StructWithUnion, \
    NegativeFieldId, Required

from forward_compatibility_fastproto.ttypes import \
    OldStructure, NewStructure, \
    OldStructureNested, NewStructureNested, \
    OldStructureNestedNested, NewStructureNestedNested


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
                                          obj.isUnion()], utf8strings=0,
                         protoid=self.PROTO)
        self.assertEqual(obj, obj_new)
        # Verify the entire buffer is read
        self.assertEqual(len(trans._readBuffer.read()), 0)
        if split != 1.0:
            self.assertEqual(1, trans.refill_called)

    def encode_and_decode(self, obj):
        trans = TMemoryBuffer()
        if self.PROTO == 0:
            proto = TBinaryProtocol.TBinaryProtocol(trans)
        else:
            proto = TCompactProtocol.TCompactProtocol(trans)

        obj.write(proto)

        obj_new = obj.__class__()
        trans = TMemoryBuffer(trans.getvalue())
        proto = proto.__class__(trans)

        obj_new.read(proto)

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
            aString=b'\x00hello',
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
        for idx in range(1, 100):
            self.decode_helper(self.buildOneOfEachB(), split=0.01 * idx)

    def test_negative_fid(self):
        self.encode_helper(NegativeFieldId(anInteger=20,
                                           aString='hello', aDouble=1.2))
        self.decode_helper(NegativeFieldId(anInteger=344444,
                                           aString=b'hello again', aDouble=1.34566))

    def test_empty_container(self):
        self.encode_helper(OneOfEach(aSet=set(), aList=[], aMap={}))
        self.decode_helper(OneOfEach(aSet=set(), aList=[], aMap={}))

    def test_required(self):
        with self.assertRaises(TProtocol.TProtocolException):
            aStruct = AStruct(anInteger=1)
            self.encode_and_decode(aStruct)

        with self.assertRaises(TProtocol.TProtocolException):
            aStruct = AStruct(aString="")
            required = Required(aStruct=aStruct)
            self.encode_and_decode(required)

    def createProto(self, trans):
        if self.PROTO == 0:
            return TBinaryProtocol.TBinaryProtocol(trans)
        else:
            return TCompactProtocol.TCompactProtocol(trans)

    def test_forward_compatibility(self):
        obj = OldStructure()
        obj.features = {}
        obj.features[1] = 314
        obj.features[2] = 271

        trans = TMemoryBuffer()
        proto = self.createProto(trans)
        obj.write(proto)

        obj_new = NewStructure()
        trans = TMemoryBuffer(trans.getvalue())
        proto = proto.__class__(trans)

        fastproto.decode(obj_new, trans, [obj_new.__class__, obj_new.thrift_spec,
                                          obj_new.isUnion()], utf8strings=0,
                         protoid=self.PROTO,
                         forward_compatibility=True)
        self.assertAlmostEqual(obj_new.features[1], 314.0)
        self.assertAlmostEqual(obj_new.features[2], 271.0)

        trans2 = TMemoryBuffer()
        proto2 = self.createProto(trans2)
        obj_new.write(proto2)

        obj_new2 = NewStructure()
        trans2 = TMemoryBuffer(trans2.getvalue())
        proto2 = proto2.__class__(trans2)

        fastproto.decode(obj_new2, trans2, [obj_new2.__class__, obj_new2.thrift_spec,
                                            obj_new2.isUnion()], utf8strings=0,
                         protoid=self.PROTO)
        self.assertAlmostEqual(obj_new2.features[1], 314.0)
        self.assertAlmostEqual(obj_new2.features[2], 271.0)

    def test_forward_compatibility_nested(self):
        obj = OldStructureNested()
        obj.features = [{}]
        obj.features[0][1] = 314
        obj.features[0][2] = 271

        trans = TMemoryBuffer()
        proto = self.createProto(trans)
        obj.write(proto)

        obj_new = NewStructureNested()
        trans = TMemoryBuffer(trans.getvalue())
        proto = proto.__class__(trans)

        fastproto.decode(obj_new, trans, [obj_new.__class__, obj_new.thrift_spec,
                                          obj_new.isUnion()], utf8strings=0,
                         protoid=self.PROTO,
                         forward_compatibility=True)
        self.assertAlmostEqual(obj_new.features[0][1], 314.0)
        self.assertAlmostEqual(obj_new.features[0][2], 271.0)

        trans2 = TMemoryBuffer()
        proto2 = self.createProto(trans2)
        obj_new.write(proto2)

        obj_new2 = NewStructureNested()
        trans2 = TMemoryBuffer(trans2.getvalue())
        proto2 = proto2.__class__(trans2)

        fastproto.decode(obj_new2, trans2, [obj_new2.__class__, obj_new2.thrift_spec,
                                            obj_new2.isUnion()], utf8strings=0,
                         protoid=self.PROTO)
        self.assertAlmostEqual(obj_new2.features[0][1], 314.0)
        self.assertAlmostEqual(obj_new2.features[0][2], 271.0)

    def test_forward_compatibility_nested_nested(self):
        obj = OldStructureNested()
        obj.features = [{}]
        obj.features[0][1] = 314
        obj.features[0][2] = 271

        objN = OldStructureNestedNested()
        objN.field = obj

        trans = TMemoryBuffer()
        proto = self.createProto(trans)
        objN.write(proto)

        obj_new = NewStructureNestedNested()
        trans = TMemoryBuffer(trans.getvalue())
        proto = proto.__class__(trans)

        fastproto.decode(obj_new, trans, [obj_new.__class__, obj_new.thrift_spec,
                                          obj_new.isUnion()], utf8strings=0,
                         protoid=self.PROTO,
                         forward_compatibility=True)
        self.assertAlmostEqual(obj_new.field.features[0][1], 314.0)
        self.assertAlmostEqual(obj_new.field.features[0][2], 271.0)

        trans2 = TMemoryBuffer()
        proto2 = self.createProto(trans2)
        obj_new.write(proto2)

        obj_new2 = NewStructureNestedNested()
        trans2 = TMemoryBuffer(trans2.getvalue())
        proto2 = proto2.__class__(trans2)

        fastproto.decode(obj_new2, trans2, [obj_new2.__class__, obj_new2.thrift_spec,
                                            obj_new2.isUnion()], utf8strings=0,
                         protoid=self.PROTO)
        self.assertAlmostEqual(obj_new2.field.features[0][1], 314.0)
        self.assertAlmostEqual(obj_new2.field.features[0][2], 271.0)


class FastBinaryTest(AbstractTest, unittest.TestCase):
    PROTO = 0


class FastCompactTest(AbstractTest, unittest.TestCase):
    PROTO = 2


if __name__ == "__main__":
    unittest.main()
