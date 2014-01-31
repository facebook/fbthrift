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
import json
import unittest
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.protocol import TSimpleJSONProtocol
from thrift.test.UnionTest.ttypes import *

class TestUnionStructs(unittest.TestCase):
    def test_init(self):
        u = TestUnion()
        self.assertEquals(TestUnion.__EMPTY__, u.getType())

        v = TestUnion(string_field='test')
        self.assertEquals(TestUnion.STRING_FIELD, v.getType())
        self.assertEquals('test', v.value)
        self.assertNotEqual(u, v)

    def test_get_set(self):
        u = TestUnion()
        u.set_i32_field(10)
        self.assertEquals(10, u.get_i32_field())

        v = TestUnion(i32_field=10)
        self.assertEquals(u, v)

        self.assertRaises(AssertionError, u.get_other_i32_field)

    def _test_json(self, j, v):
        u = TestUnion()
        u.readFromJson(json.dumps(j))
        self.assertEquals(v, u)

    def test_json(self):
        v = TestUnion(i32_field=123)
        j = {'i32_field': 123}
        self._test_json(j, v)

        v = TestUnion(string_field='test')
        j = {'string_field': 'test'}
        self._test_json(j, v)

    def _test_read_write(self, u):
        protocol_factory = TBinaryProtocol.TBinaryProtocolAcceleratedFactory()
        databuf = TTransport.TMemoryBuffer()
        prot = protocol_factory.getProtocol(databuf)
        u.write(prot)

        ndatabuf = TTransport.TMemoryBuffer(databuf.getvalue())
        prot = protocol_factory.getProtocol(ndatabuf)
        v = TestUnion()
        v.read(prot)
        self.assertEquals(u, v)

    def test_read_write(self):
        l = [
            TestUnion(string_field='test'),
            TestUnion(),
            TestUnion(i32_field=100),
        ]

        for i in l:
            self._test_read_write(i)

    def _test_json_output(self, u, j):
        protocol_factory = TSimpleJSONProtocol.TSimpleJSONProtocolFactory()
        databuf = TTransport.TMemoryBuffer()
        prot = protocol_factory.getProtocol(databuf)
        u.write(prot)

        self.assertEquals(j, json.loads(databuf.getvalue()))

    def test_json_output(self):
        l = [
          (TestUnion(), {}),
          (TestUnion(i32_field=10), {'i32_field': 10}),
          (TestUnion(string_field='test'), {'string_field': 'test'})
        ]

        for i, j in l:
          self._test_json_output(i, j)


if __name__ == '__main__':
    unittest.main()
