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

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import sys, glob, json
sys.path.insert(0, './gen-py')
lib_path = glob.glob('../../lib/py/build/lib.*')
if lib_path:
  sys.path.insert(0, lib_path[0])

from ThriftTest.ttypes import *
from thrift.transport import TTransport
from thrift.transport import TSocket
from thrift.protocol import TBinaryProtocol
from thrift.protocol import TJSONProtocol
from thrift.protocol import TSimpleJSONProtocol
from thrift.util import Serializer

import unittest
import time

class AbstractTest():

  def setUp(self):
      self.v1obj = VersioningTestV1(
          begin_in_both=12345,
          old_string=b'aaa',
          end_in_both=54321,
          )

      self.v2obj = VersioningTestV2(
          begin_in_both=12345,
          newint=1,
          newbyte=2,
          newshort=3,
          newlong=4,
          newdouble=5.0,
          newstruct=Bonk(message=b"Hello!", type=123),
          newlist=[7,8,9],
          newset=[42,1,8],
          newmap={1:2,2:3},
          newstring=b"Hola!",
          newunicodestring=u"any\x7f\xff".encode('utf-8'),
          newbool=True,
          end_in_both=54321,
          )

      self.sjtObj = SimpleJSONTestStruct(
        m={
          1:self.v1obj,
          2:self.v2obj,
        },
      )

  def _serialize(self, obj):
      return Serializer.serialize(self.protocol_factory, obj)

  def _deserialize(self, objtype, data):
      return Serializer.deserialize(self.protocol_factory, data, objtype())

  def testSimpleJSON(self):
      if not isinstance(self, SimpleJSONTest):
          return
      json.loads(self._serialize(self.sjtObj))
      json.loads(self._serialize(self.v1obj))
      json.loads(self._serialize(self.v2obj))

  def testForwards(self):
      if isinstance(self, SimpleJSONTest):
          return
      obj = self._deserialize(VersioningTestV2, self._serialize(self.v1obj))
      self.assertEquals(obj.begin_in_both, self.v1obj.begin_in_both)
      self.assertEquals(obj.end_in_both, self.v1obj.end_in_both)

  def testUnicodeString(self):
      if isinstance(self, SimpleJSONTest):
          return
      obj = self._deserialize(VersioningTestV2, self._serialize(self.v2obj))
      if isinstance(self, JSONProtocolTest):
          self.assertEquals(obj.newunicodestring,
                            self.v2obj.newunicodestring.decode('utf-8'))
      else:
          self.assertEquals(obj.newunicodestring, self.v2obj.newunicodestring)

  def testBackwards(self):
      if isinstance(self, SimpleJSONTest):
          return
      obj = self._deserialize(VersioningTestV1, self._serialize(self.v2obj))
      self.assertEquals(obj.begin_in_both, self.v2obj.begin_in_both)
      self.assertEquals(obj.end_in_both, self.v2obj.end_in_both)


class NormalBinaryTest(AbstractTest, unittest.TestCase):
  protocol_factory = TBinaryProtocol.TBinaryProtocolFactory()

class AcceleratedBinaryTest(AbstractTest, unittest.TestCase):
  protocol_factory = TBinaryProtocol.TBinaryProtocolAcceleratedFactory()


class AcceleratedFramedTest(unittest.TestCase):
  def testSplit(self):
    """Test FramedTransport and BinaryProtocolAccelerated

    Tests that TBinaryProtocolAccelerated and TFramedTransport
    play nicely together when a read spans a frame"""

    protocol_factory = TBinaryProtocol.TBinaryProtocolAcceleratedFactory()
    bigstring = "".join(chr(byte) for byte in range(ord("a"), ord("z")+1))

    databuf = TTransport.TMemoryBuffer()
    prot = protocol_factory.getProtocol(databuf)
    prot.writeI32(42)
    prot.writeString(bigstring)
    prot.writeI16(24)
    data = databuf.getvalue()
    cutpoint = len(data) // 2
    parts = [ data[:cutpoint], data[cutpoint:] ]

    framed_buffer = TTransport.TMemoryBuffer()
    framed_writer = TTransport.TFramedTransport(framed_buffer)
    for part in parts:
      framed_writer.write(part)
      framed_writer.flush()
    self.assertEquals(len(framed_buffer.getvalue()), len(data) + 8)

    # Recreate framed_buffer so we can read from it.
    framed_buffer = TTransport.TMemoryBuffer(framed_buffer.getvalue())
    framed_reader = TTransport.TFramedTransport(framed_buffer)
    prot = protocol_factory.getProtocol(framed_reader)
    self.assertEqual(prot.readI32(), 42)
    self.assertEqual(prot.readString(), bigstring)
    self.assertEqual(prot.readI16(), 24)

class SimpleJSONTest(AbstractTest):
  protocol_factory = TSimpleJSONProtocol.TSimpleJSONProtocolFactory()

class JSONProtocolTest(AbstractTest, unittest.TestCase):
  protocol_factory = TJSONProtocol.TJSONProtocolFactory()

def suite():
  suite = unittest.TestSuite()
  loader = unittest.TestLoader()

  suite.addTest(loader.loadTestsFromTestCase(NormalBinaryTest))
  suite.addTest(loader.loadTestsFromTestCase(AcceleratedBinaryTest))
  suite.addTest(loader.loadTestsFromTestCase(AcceleratedFramedTest))
  suite.addTest(loader.loadTestsFromTestCase(SimpleJSONTest))
  suite.addTest(loader.loadTestsFromTestCase(JSONProtocolTest))
  return suite

if __name__ == "__main__":
  unittest.main(defaultTest="suite", testRunner=unittest.TextTestRunner(verbosity=2))
