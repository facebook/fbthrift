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

from thrift.transport.TTransport import TMemoryBuffer
from JsonReaderTest.ttypes import StructContainingOptionalList
from JsonReaderTest.ttypes import StructContainingRequiredList
from thrift.protocol.TProtocol import TProtocolException

class TestJSONReader(unittest.TestCase):

    def testReadNullOptionalList(self):
        struct = StructContainingOptionalList()
        struct.readFromJson('{ "data" : null }')

    def testReadNullRequiredList(self):
        try:
            struct = StructContainingRequiredList()
            struct.readFromJson('{ "data" : null }')
            self.assertFalse(True,
                    "Should have failed with required field missing")
        except TProtocolException as ex:
            pass

if __name__ == '__main__':
    unittest.main()
