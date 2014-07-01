#!/usr/local/bin/python2.6 -tt
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

import logging
import sys
import unittest

from cStringIO import StringIO
from cPickle import loads, dumps

from thrift_compiler.generate.t_output import IndentedOutput
from thrift_compiler.generate.t_output_aggregator import get_global_scope
from thrift_compiler.generate.t_output_aggregator import OutputContext
from thrift_compiler.generate.t_cpp_context import CppOutputContext
from thrift_compiler.generate.t_cpp_context import CppPrimitiveFactory

def print_buffers(buffers):
    log = logging.getLogger("TestPickle.print_buffers")
    for name, buf in buffers.iteritems():
        log.debug(name)
        log.debug('-' * 80)
        log.debug(buf.getvalue())
        log.debug('-' * 80)

class TestPickle(unittest.TestCase):
    def make_test_scope(self):
        sg = get_global_scope(CppPrimitiveFactory)
        sg('using namespace something;')
        sg()
        s = sg.namespace('apache.thrift').scope
        with s.cls('class A') as s1:
            s1.label('public:')
            with s1.defn('{name}()', name='A') as s2:
                s3 = s2('if (all good with ya)').scope
                s3('some instruction;')
                s3('some other instruction;')
                # Referencing the scope is enough to create the scope
                s3('while (false)').scope
            s1()
            s1.label('private:')
            s1('// forbidden copy constructor')
            s1.defn('{name}(const {class}& rhs)', name='A')
            s1('// forbidden assignment operator')
            s1.defn('{name}& operator = (const {class}& rhs)', name='A')

        self.assertTrue(all(child.parent is sg  for child in sg._children))
        return sg

    def produce_output(self, scope):
        # create two outputs for the cpp and header, and commit this scope to
        # the CppOutputContext that wraps them
        bufcpp = StringIO()
        bufh = StringIO()
        buftcc = StringIO()
        context = CppOutputContext(IndentedOutput(bufcpp),
            IndentedOutput(bufh), IndentedOutput(buftcc), 'test.h')
        # now write it to the context
        scope.commit(context)
        return dict(cpp=bufcpp, h=bufh)

    def assertBuffersEqual(self, bufs1, bufs2):
        self.assertTrue(all(bufs1[k].getvalue() == bufs2[k].getvalue() \
                            for k in bufs1.iterkeys()))

    def testPickleUnpickleAndCompareOutputs(self):
        sg = self.make_test_scope()
        serialized = dumps(sg)
        sg2 = loads(serialized)
        buffers1 = self.produce_output(sg)
        buffers2 = self.produce_output(sg2)
        # assert that the unpickled program tree (sg2) produces identical files
        self.assertBuffersEqual(buffers1, buffers2)
        print_buffers(buffers2)

if __name__ == '__main__':
    logging.basicConfig(stream=sys.stdout)
    logging.getLogger("TestPickle.print_buffers").setLevel(logging.DEBUG)
    unittest.main()
