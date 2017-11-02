import unittest


from thrift.protocol.TCompactProtocol import TCompactProtocol
from thrift.transport.TTransport import TMemoryBuffer
from thrift.util.BytesStrIO import BytesStrIO as StringIO


class TestCompactProtocolSerialization(unittest.TestCase):
    def test_primitive_serialization(self):
        val = int(12)
        buf = TMemoryBuffer()
        proto = TCompactProtocol(buf)
        proto.writeI32(val)
        reader = TCompactProtocol(StringIO(buf.getvalue()))
        self.assertEqual(reader.readI32(), val)
