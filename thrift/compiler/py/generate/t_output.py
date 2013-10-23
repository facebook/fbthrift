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

class Output:
    'Output interface'

    def __init__(self, output):
        self._on_blank_line = True
        self._output = output
        self._flag = False

    def write(self, lines):
        'Writes lines (a string) '
        # assume we are not on a blank line
        self._write(lines)

    def _write(self, lines):
        raise NotImplementedError

    def _write_line_contents(self, line):
        # if anything was written
        if self._write_line_contents_impl(line):
            self._flag = False
            self._on_blank_line = False

    def _write_line_contents_impl(self, line):
        'Returns whether anything was written or not'
        raise NotImplementedError

    def indent(self):
        pass

    def unindent(self):
        pass

    def force_newline(self):
        'Force a newline'
        print >>self._output
        self._on_blank_line = True
        self._flag = False

    def line_feed(self):
        '''Enforce that the cursor is placed on an empty line, returns whether
        a newline was printed or not'''
        if not self._on_blank_line:
            self.force_newline()
            return True
        return False

    def double_space(self):
        '''Unless current position is already on an empty line, insert a blank
        line before the next statement, as long as it's not the beginning of
        a statement'''
        ofl = self.on_flagged_line
        if self.line_feed() and not ofl:
            self.force_newline()

    def flag_this_line(self, flag=True):
        '''Mark the current line as flagged. This is used to apply specific
        logic to certain lines, such as those that represent the start of a
        scope.'''
        # TODO(dsanduleac): deprecate this and use a num_lines_printed
        # abstraction instead. Then modify t_cpp_generator to always print
        # newlines after statements. Should modify the behaviour of
        # double_space as well.
        self._flag = flag

    @property
    def on_flagged_line(self):
        'Returns whether the line we are currently on has been flagged'
        return self._flag


class IndentedOutput(Output):

    def __init__(self, output, indent_char=' '):
        Output.__init__(self, output)
        self._indent = 0
        # configurable values
        self._indent_char = indent_char

    def indent(self, amount):
        assert self._indent + amount >= 0
        self._indent += amount

    def unindent(self, amount):
        assert self._indent - amount >= 0
        self._indent -= amount

    def _write_line_contents_impl(self, line):
        # strip trailing characters
        line = line.rstrip()
        # handle case where _write is being passed a \n-terminated string
        if line == '':
            # don't flag the current line as non-empty, we didn't write
            # anything
            return False
        if self._on_blank_line:
            # write indent
            self._output.write(self._indent * self._indent_char)
        # write content
        self._output.write(line)
        return True

    def _write(self, lines):
        lines = lines.split('\n')
        self._write_line_contents(lines[0])
        for line in lines[1:]:
            # ensure newlines between input lines
            self.force_newline()
            self._write_line_contents(line)


class CompositeOutput(Output):
    def __init__(self, *outputs):
        self._outputs = outputs

    def indent(self):
        for output in self._outputs:
            output.indent()

    def unindent(self):
        for output in self._outputs:
            output.unindent()

    def _write(self, lines):
        for output in self._outputs:
            output.write(lines)

    def _write_line_contents_impl(self, line):
        # this is never used internally in this output
        pass

    def line_feed(self):
        for output in self._outputs:
            output.line_feed()

    def force_newline(self):
        for output in self._outputs:
            output.force_newline()

    # These two don't make sense to be implemented here

    def flag_this_line(self):
        raise NotImplementedError

    def on_flagged_line(self):
        raise NotImplementedError

    def double_space(self):
        for output in self._outputs:
            output.double_space()


class DummyOutput(Output):
    def __init__(self):
        pass

    def _write(self, lines):
        pass

    def _write_line_contents_impl(self, line):
        pass

    def force_newline(self):
        pass

    def line_feed(self):
        pass

    def double_space(self):
        pass

    def flag_this_line(self, flag=True):
        pass

    @property
    def on_flagged_line(self):
        return False
