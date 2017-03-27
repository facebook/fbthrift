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
import textwrap
import unittest
from thrift.test.UnionTest.ttypes import \
    RandomStuff, \
    StructWithUnionAndOther, \
    TestUnion


class TestRepr(unittest.TestCase):

    def assertReprEquals(self, obj):
        self.assertEquals(obj, eval(repr(obj)))

    def test_repr(self):
        """ Ensure that __repr__() return a valid expression that can be
        used to construct the original object
        """

        self.assertReprEquals(RandomStuff(bigint=123456))

        self.assertReprEquals(StructWithUnionAndOther(string_field="test"))

        self.assertReprEquals(TestUnion(string_field="blah"))

    def test_content(self):
        """ Ensure that the content of repr() is what we wanted. We should
        print the member in the same order as it is appeared in Thrift file.
        """
        obj = RandomStuff(bigint=123)
        output = """\
            RandomStuff(
                a=None,
                b=None,
                c=None,
                d=None,
                myintlist=None,
                bigint=123,
                triple=None)"""
        self.assertEquals(repr(obj), textwrap.dedent(output))
