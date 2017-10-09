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

import unittest

from enums_py3.types import MyEnum1, EmptyEnum


class EnumTest(unittest.TestCase):
    def test_foo(self):
        # Test EmptyEnum works, doesn't cause errors.
        print(EmptyEnum.__dict__)

    def test_values(self):
        # Check that all the enum values match what we expect
        my_enum1_values = [x.value for x in MyEnum1]
        self.assertEquals(min(my_enum1_values), 0)
        self.assertEquals(MyEnum1.ME1_0, 0)
        self.assertEquals(MyEnum1.ME1_1, 1)
        self.assertEquals(MyEnum1.ME1_2, 2)
        self.assertEquals(MyEnum1.ME1_3, 3)
        self.assertEquals(MyEnum1.ME1_5, 5)
        self.assertEquals(MyEnum1.ME1_6, 6)
        self.assertEquals(max(my_enum1_values), 6)

    def test_enum_names(self):
        self.assertEquals(MyEnum1.ME1_0.name, "ME1_0")

    def test_names_to_values(self):
        self.assertEquals(MyEnum1["ME1_0"], MyEnum1.ME1_0)

    def test_compare(self):
        self.assertTrue(MyEnum1.ME1_0 < MyEnum1.ME1_1)


if __name__ == '__main__':
    unittest.main()
