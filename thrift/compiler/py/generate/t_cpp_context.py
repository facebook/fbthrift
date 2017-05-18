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

import re

from t_output import CompositeOutput
from t_output_aggregator import create_scope_factory
from t_output_aggregator import OutputContext
from t_output_aggregator import Primitive
from t_output_aggregator import PrimitiveFactory
from t_output_aggregator import Scope

# ---------------------------------------------------------------
# Scope
# ---------------------------------------------------------------

class CppScope (Scope):

    # Make sure the line is flagged only when an open brace was printed and
    # while it wasn't closed

    def acquire(self):
        print >>self._out, ' {',
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

class Definition(Primitive):
    def _write(self, context):
        # TODO error in case badly formatted (str(self) doesn't contain
        # {name})
        fields = {}
        if 'symbol_name' in self:
            fields['symbol_name'] = self.symbol_name
        if 'result_type' in self:
            fields['result_type'] = self.result_type
        fields['symbol_scope'] = ''
        tpl_default_n = 0
        tpl_default_list = []
        while 'tpl_default_' + str(tpl_default_n) in self:
            s = 'tpl_default_' + str(tpl_default_n)
            tpl_default_list.append(s)
            fields[s] = '= ' + str(getattr(self, s))
            tpl_default_n = tpl_default_n + 1

        if 'name' not in self:
            if not self.in_header:
                raise AttributeError('Cannot find mandatory keyword "name"'
                                        ' inside defn: ' + repr(self))
            txt = str(self)
        else:
            fields['name'] = self.name
            # if we're in a class, define the {class} field to its name
            if 'abspath' in self.parent.opts:
                fields['class'] = self.parent.opts.abspath

            txt = str(self).format(**fields)

        if self.in_header:
            context.h.double_space()
        else:
            context.h.line_feed()

        if 'modifiers' in self:
            txtsplit = txt.split('\n')
            # Ensure that templates end up before static or other modifiers
            if len(txtsplit) > 1:
                txt = txtsplit[0] + '\n' + self.modifiers + \
                    ' ' + "\n".join(txtsplit[1:])
            else:
                txt = self.modifiers + ' ' + txt
        context.h.write(txt)

        write_defn = True
        if self.pure_virtual:
            context.h.write(" = 0;")
            write_defn = False
        elif self.override:
            context.h.write(" override")
        elif self.default:
            context.h.write(" = default;")
            write_defn = False
        elif self.delete:
            context.h.write(" = delete;")
            write_defn = False
        elif self.no_except:
            context.h.write(" noexcept")

        if not self.in_header and write_defn:
            # no custom epilogue? we'll just set our own haha
            if 'epilogue' not in self:
                self.epilogue = '\n\n'
            print >>context.h, ';',
            if not 'abspath' in self.parent.opts:
                # We're not inside of a class so just ignore namespacing
                # Use previously defined txt
                pass
            else:
                for i in tpl_default_list:
                    fields[i] = ''
                fields['symbol_scope'] = self.parent.opts.abspath + '::'
                fields['name'] = ''.join((self.parent.opts.abspath,
                    '::', self.name))
                txt = str(self).format(**fields)
            decl_output = self.output or context.impl
            # make sure this parameter is set and not assumed
            self.in_header = False
            # delegate writing of the implementation signature til we
            # actually open its scope
            self.impl_signature = txt
        # self.in_header => write the declaration in the header as well
        else:
            decl_output = context.h
        if self.init_dict:
            if 'value' in self:
                raise AttributeError("Can't have both value and init_dict"
                    " in a defn. It's either a constructor"
                    " or it has a value.")
            if not self.in_header:
                raise AttributeError("Constructors (definitions that "
                    "include init_dict) have to have in_header set.")
            init_txt = ' : \n    ' + ',\n    '.join("{0}({1})".format(*x)
                                for x in self.init_dict.iteritems())
            decl_output.write(init_txt)
        if 'value' in self:
            # well, we don't have to store the self.impl_signature
            # anymore.
            decl_output.line_feed()
            decl_output.write(self.impl_signature)
            del self._opts['impl_signature']
            decl_output.write(self.value)
        # set its scope's output to decl_output (self.output will get
        # passed to the scope into self.scope.opts.output.. jsyk).
        self.output = decl_output

    def enter_scope_callback(self, context, scope):
        if not scope.opts.in_header:
            scope.opts.output.line_feed()
            scope.opts.output.write(scope.opts.impl_signature)


class Class(Primitive):
    # String Format: type folly abspath::name
    # Example: class FOLLY_DEPRECATE("msg") classname::function : extrastuff
    _pattern_type = "(?P<type>class |struct )"
    _pattern_folly = "(?P<folly>\w+\(.*?\) )*"
    _pattern_name = "(?:\s*(?P<name>\w+))"
    _pattern_scope = "(?:\s*::{pname})*".format(pname=_pattern_name)
    _pattern_abspath = "(?P<abspath>\w+{pscope})".format(pscope=_pattern_scope)
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
        # this is magic! Basically what it does it it checks if we're
        # already on an empty line. If we are not then we introduce a
        # newline before the class defn
        context.h.double_space()
        if context.impl:
            context.impl.double_space()
        print >>context.h, self,
        # the scope of this will be written to output_h
        self.output = context.h
        # no custom epilogue? we'll just set our own haha
        if 'epilogue' not in self:
            self.epilogue = ';'
            # basically force two newlines after a class definition if it's
            # toplevel (not within another class)
            if not issubclass(self.parent.opts.type, Class):
                self.epilogue += '\n\n'


class Label(Primitive):
    def _write(self, context):
        assert issubclass(self.parent.opts.type, Class), \
            'Declaring a label not inside a class'
        context.output.line_feed()
        context.output.unindent(1)
        print >>context.output, self
        context.output.indent(1)


class Extern(Primitive):
    def _write(self, context):
        context.h.line_feed()
        context.impl.line_feed()
        print >>context.h, "extern {0};".format(str(self)),
        self.output = context.impl
        print >>context.impl, str(self),
        if 'value' in self:
            print >>context.impl, self.value,
        else:
            print >>context.impl, ';',
        # the epilogue for the scope (which will be in output_cpp)
        self.epilogue = ";\n\n"


class ImplOnlyStatement(Primitive):
    def _write(self, context):
        # set the scope's output to cpp
        self.output = context.impl
        self.output.line_feed()
        self.output.write(str(self))
        self.epilogue = ";\n\n"


class SameLineStmt(Primitive):
    def _write(self, context):
        txt = ' ' + str(self)
        context.output.write(txt)


class Case(Primitive):
    def _write(self, context):
        # should assert issubclass(self.parent.opts.type, Switch), but cbb
        # plus we don't have a 'Switch' type so maybe some other time
        context.output.line_feed()
        if str(self) == 'default':
            print >>context.output, 'default:',
        else:
            print >>context.output, 'case {0}:'.format(str(self)),

    def enter_scope_callback(self, context, scope):
        context.output.line_feed()
        print >>context.output, '{',
        context.output.indent(2)
        return dict(physical_scope=False)

    def exit_scope_callback(self, context, scope):
        context.output.line_feed()
        if 'nobreak' not in self or not self.nobreak:
            print >>context.output, 'break;'
        context.output.unindent(2)
        print >>context.output, '}',
        return dict(physical_scope=False)


class Statement(Primitive):
    def _write(self, context):
        txt = str(self)
        # statements always start on new lines
        context.output.line_feed()
        context.output.write(txt)


class Catch(Primitive):
    def _write(self, context):
        txt = str(self)
        context.output.line_feed()
        context.output.unindent(2)
        if len(txt) == 0:
            print >> context.output, "} catch {"
        else:
            print >> context.output, "} catch(" + txt + ") {"
        context.output.indent(2)

    def enter_scope_callback(self, context, scope):
        return dict(physical_scope=False)

    def exit_scope_callback(self, context, scope):
        context.output.line_feed()
        return dict(physical_scope=False)


class Namespace(Primitive):
    def __init__(self, parent, path):
        super(Namespace, self).__init__(parent, text=None, path=path)
        self.epilogue = None

    def _write(self, context):
        path = filter(None, self.path)
        if path:
            parts = [r'namespace {0} {{'.format(i) for i in path]
            text = ' '.join(parts) + '\n'
            self.epilogue = '}' * len(path) + ' // ' + '::'.join(path)
            context.outputs.line_feed()
            print >>context.outputs, text

    def enter_scope_callback(self, context, scope):
        return dict(physical_scope=False)

    def exit_scope_callback(self, context, scope):
        if scope.opts.epilogue:
            # namespaces don't have physical_scope cause they have an ending
            # text hardcoded into .epilogue by the write_primitive method
            context.outputs.double_space()
            # => write the epilogue statement for all outputs
            print >>context.outputs, scope.opts.epilogue,
        return dict(physical_scope=False)


class CppPrimitiveFactory(PrimitiveFactory):
    # TODO enforce somehow that each PrimitiveFactory subclass defines a types
    # staticvar (method_name => class to instantiate with default parameters)
    types = dict(case=Case, defn=Definition, cls=Class, label=Label,
                 catch=Catch,
                 extern=Extern, impl=ImplOnlyStatement, sameLine=SameLineStmt)

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

    def __init__(self, output_cpp, output_h, output_tcc, header_path,
            additional_outputs=[], custom_protocol_h=None):
        self.omit_include = False
        self._output_cpp = output_cpp
        self._output_h = output_h
        self._output_tcc = output_tcc
        self._additional_outputs = additional_outputs
        self._custom_protocol_h = custom_protocol_h
        self._header_path = header_path
        outputs = [output_h]
        if output_cpp:
            outputs.append(output_cpp)
        if output_tcc:
            outputs.append(output_tcc)
        outputs.extend(additional_outputs)

        for output in outputs:
            output.make_scope = create_scope_factory(CppScope, output)

        # shorthand to write to all outputs at the same time
        self._all_outputs = CompositeOutput(*outputs)
        # start writing in the header
        self.output = output_h

    @property
    def h(self):
        return self._output_h

    @property
    def additional_outputs(self):
        return self._additional_outputs

    @property
    def custom_protocol_h(self):
        return self._custom_protocol_h

    @property
    def impl(self):
        return self._output_cpp

    @property
    def tcc(self):
        return self._output_tcc

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
            print >>self._output_h, '#pragma once\n'
            if self._custom_protocol_h is not None:
                custom_protocol_comment = '''
/**
 * This header file includes the tcc files of the corresponding header file
 * and the header files of its dependent types. Include this header file
 * only when you need to use custom protocols (e.g. DebugProtocol,
 * VirtualProtocol) to read/write thrift structs.
 */
'''
                print >>self._custom_protocol_h, '#pragma once\n'
                print >>self._custom_protocol_h, custom_protocol_comment
                print >>self._custom_protocol_h, '#include "{0}.tcc"\n'.format(
                        self._header_path)
            # include h in cpp
            if not self.omit_include:
                if self._output_cpp:
                    print >>self._output_cpp, '#include "{0}.h"\n'.format(
                            self._header_path)
            for output in self._additional_outputs:
                print >>output, '#include "{0}.h"\n'.format(
                    self._header_path)
            if self._output_tcc:
                if self._output_cpp:
                    print >>self._output_cpp, '#include "{0}.tcc"\n'.format(
                        self._header_path)
                for output in self._additional_outputs:
                    print >>output, '#include "{0}.tcc"\n'.format(
                        self._header_path)
                # start guard in tcc
                print >>self._output_tcc, '#pragma once\n'
                # include h in tcc
                print >>self._output_tcc, '#include "{0}.h"'.format(
                            self._header_path)
                print >>self._output_tcc, \
                        '#include <thrift/lib/cpp/TApplicationException.h>'
                print >>self._output_tcc, '#include <folly/io/IOBuf.h>'
                print >>self._output_tcc, '#include <folly/io/IOBufQueue.h>'
                print >>self._output_tcc, \
                        '#include <thrift/lib/cpp/transport/THeader.h>'
                print >>self._output_tcc, \
                        '#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>'
                print >>self._output_tcc, \
                        '#include <thrift/lib/cpp2/GeneratedCodeHelper.h>'
                print >>self._output_tcc, \
                        '#include <thrift/lib/cpp2/GeneratedSerializationCodeHelper.h>'
                print >>self._output_tcc, ''
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
