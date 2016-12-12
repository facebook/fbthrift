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

import unittest

from thrift.protocol import TSimpleJSONProtocol
from thrift.transport.TTransport import TMemoryBuffer

# The sorted one.
from SortKeys.ttypes import SortedStruct


def writeToJSON(obj):
    trans = TMemoryBuffer()
    proto = TSimpleJSONProtocol.TSimpleJSONProtocol(trans)
    obj.write(proto)
    return trans.getvalue()


def readStructFromJSON(jstr, struct_type):
    stuff = struct_type()
    trans = TMemoryBuffer(jstr)
    proto = TSimpleJSONProtocol.TSimpleJSONProtocol(trans,
                                                    struct_type.thrift_spec)
    stuff.read(proto)
    return stuff


class TestSortKeys(unittest.TestCase):
    def testSorted(self):
        static_struct = SortedStruct(aMap={'b': 1.0, 'a': 1.0})
        unsorted_blob = b'{\n  "aMap": {\n    "b": 1.0,\n    "a": 1.0\n  }\n}'
        sorted_blob = b'{\n  "aMap": {\n    "a": 1.0,\n    "b": 1.0\n  }\n}'
        sorted_struct = readStructFromJSON(unsorted_blob, SortedStruct)
        blob = writeToJSON(sorted_struct)
        self.assertNotEqual(blob, unsorted_blob)
        self.assertEqual(blob, sorted_blob)
        self.assertEqual(static_struct, sorted_struct)
