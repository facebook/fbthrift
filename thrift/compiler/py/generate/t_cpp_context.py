#! /usr/bin/env python3
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

from __future__ import print_function

import re

from .t_output import CompositeOutput
from .t_output_aggregator import create_scope_factory
from .t_output_aggregator import OutputContext
from .t_output_aggregator import Primitive
from .t_output_aggregator import PrimitiveFactory
from .t_output_aggregator import Scope

# ---------------------------------------------------------------
# Scope
# ---------------------------------------------------------------

class CppScope (Scope):

    # Make sure the line is flagged only when an open brace was printed and
    # while it wasn't closed

    def acquire(self):
        print(' {', end=' ', file=self._out)
        self._out.flag_this_line()
        self._out.indent(2)

    def release(self):
        self._out.unindent(2)
        if not self._out.on_flagged_line:
            self._out.line_feed()
        self._out.flag_this_line(False)
        self._out.write('}')


# ---------------------------------------------------------------
# PrimitiveFactory and primitives
# ---------------------------------------------------------------

class Class(Primitive):
    # String Format: type folly abspath::name
    # Example: class FOLLY_DEPRECATE("msg") classname::function : extrastuff
    _pattern_type = "(?P<type>class |struct )"
    _pattern_folly = r"(?P<folly>\w+\(.*?\) )*"
    _pattern_name = r"(?:\s*(?P<name>\w+))"
    _pattern_scope = r"(?:\s*::{pname})*".format(pname=_pattern_name)
    _pattern_abspath = r"(?P<abspath>\w+{pscope})".format(pscope=_pattern_scope)
    _pattern = "{ptype}{pfolly}{pabspath}".format(
        ptype=_pattern_type,
        pfolly=_pattern_folly,
        pabspath=_pattern_abspath)
    _classRegex = re.compile(_pattern, re.S)

    def _write(self, context):
        # deduce name
        m = self._classRegex.match(str(self))
        if not m:
            raise SyntaxError("C++ class/struct incorrectly defined")
        self.name, self.abspath = m.group('name', 'abspath')
        if 'abspath' in self.parent.opts and self.parent.opts.abspath:
            self.abspath = '::'.join((self.parent.opts.abspath, self.abspath))
        # this is magic! Basically what it does it it checks if we're
        # already on an empty line. If we are not then we introduce a
        # newline before the class defn
        context.h.double_space()
        print(self, end=' ', file=context.h)
        # the scope of this will be written to output_h
        self.output = context.h
        # no custom epilogue? we'll just set our own haha
        if 'epilogue' not in self:
            self.epilogue = ';'
            # basically force two newlines after a class definition if it's
            # toplevel (not within another class)
            if not issubclass(self.parent.opts.type, Class):
                self.epilogue += '\n\n'


class Statement(Primitive):
    def _write(self, context):
        txt = str(self)
        # statements always start on new lines
        context.output.line_feed()
        context.output.write(txt)


class Namespace(Primitive):
    def __init__(self, parent, path):
        super(Namespace, self).__init__(parent, text=None, path=path)
        self.epilogue = None

    def _write(self, context):
        path = [_f for _f in self.path if _f]
        if path:
            parts = [r'namespace {0} {{'.format(i) for i in path]
            text = ' '.join(parts) + '\n'
            self.epilogue = '}' * len(path) + ' // ' + '::'.join(path)
            context.outputs.line_feed()
            print(text, file=context.outputs)

    def enter_scope_callback(self, context, scope):
        return dict(physical_scope=False)

    def exit_scope_callback(self, context, scope):
        if scope.opts.epilogue:
            # namespaces don't have physical_scope cause they have an ending
            # text hardcoded into .epilogue by the write_primitive method
            context.outputs.double_space()
            # => write the epilogue statement for all outputs
            print(scope.opts.epilogue, end=' ', file=context.outputs)
        return dict(physical_scope=False)


class CppPrimitiveFactory(PrimitiveFactory):
    # TODO enforce somehow that each PrimitiveFactory subclass defines a types
    # staticvar (method_name => class to instantiate with default parameters)
    types = {'cls': Class}

    def namespace(self, ns):
        path = ns.split('.')
        return Namespace(self._scope(), path)

    def stmt(self, text='\n'):
        'non-special statement, default to newline'
        return Statement(self._scope(), text)

    __call__ = stmt

# ---------------------------------------------------------------
# OutputContext
# ---------------------------------------------------------------

class CppOutputContext(OutputContext):

    def __init__(self, output_h, header_path):
        self._output_h = output_h
        self._header_path = header_path
        outputs = [output_h]

        for output in outputs:
            output.make_scope = create_scope_factory(CppScope, output)

        # shorthand to write to all outputs at the same time
        self._all_outputs = CompositeOutput(*outputs)
        # start writing in the header
        self.output = output_h

    def __enter__(self):
        self.output.__enter__()
        return self

    def __exit__(self, *args):
        self.output.__exit__(*args)

    @property
    def h(self):
        return self._output_h

    @property
    def output(self):
        return self._output_crt

    @output.setter
    def output(self, output):
        self._output_crt = output

    @property
    def outputs(self):
        return self._all_outputs

    def _enter_scope_handler(self, scope, physical_scope=True):
        if scope.parent is None:
            # save the default "current output" in the parent scope
            scope.opts.output = self.output
            # start guard in h
            print('#pragma once\n', file=self._output_h)
            return

        # set the output of the real scope's content according to the
        # logical scope's output
        if not 'output' in scope.opts:
            # if it doesn't then it's a namespace or something, just pass
            # the output of its parent on
            scope.opts.output = scope.parent.opts.output
        self.output = scope.opts.output

        if physical_scope:
            pscope = self.output.make_scope()
            scope.physical_scope = pscope
            pscope.acquire()

    def _exit_scope_handler(self, scope, physical_scope=True):
        if scope.parent is None:
            # Make sure file is newline terminated.
            self.outputs.line_feed()
            return

        if physical_scope:
            scope.physical_scope.release()
            if 'epilogue' in scope.opts:
                self.output.write(scope.opts.epilogue)

        # reset the output to the parent scope's output
        self.output = scope.parent.opts.output
