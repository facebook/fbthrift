# Copyright (c) Facebook, Inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import os
import shutil
import subprocess
import sys
import tempfile
import textwrap
import unittest


def ascend_find_exe(path, target):
    if not os.path.isdir(path):
        path = os.path.dirname(path)
    while True:
        test = os.path.join(path, target)
        if os.access(test, os.X_OK):
            return test
        parent = os.path.dirname(path)
        if os.path.samefile(parent, path):
            return None
        path = parent

exe = os.path.join(os.getcwd(), sys.argv[0])
thrift = ascend_find_exe(exe, 'thrift')


def read_file(path):
    with open(path, 'r') as f:
        return f.read()


def write_file(path, content):
    with open(path, 'w') as f:
        f.write(content)


class CompilerFailureTest(unittest.TestCase):

    def setUp(self):
        tmp = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, tmp, True)
        self.tmp = tmp
        self.addCleanup(os.chdir, os.getcwd())
        os.chdir(self.tmp)
        self.maxDiff = None

    def run_thrift(self, *args):
        argsx = [thrift, "--gen", "cpp2"]
        argsx.extend(args)
        pipe = subprocess.PIPE
        p = subprocess.Popen(argsx, stdout=pipe, stderr=pipe)
        out, err = p.communicate()
        out = out.decode(sys.getdefaultencoding())
        err = err.decode(sys.getdefaultencoding())
        err = err.replace("{}/".format(self.tmp), "")
        return p.returncode, out, err

    def test_enum_wrong_default_value(self):
        # tests initializing enum with default value of wrong type
        write_file("foo.thrift", textwrap.dedent("""\
            enum Color {
                RED = 1,
                GREEN = 2,
                BLUE = 3,
            }

            struct MyS {
                1: Color color = -1
            }
        """))
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(ret, 0)
        self.assertEqual(
            err,
            "[WARNING:foo.thrift:9] type error: const \"color\" was declared as enum \"Color\" with a value not of that enum\n",
        )

    def test_duplicate_method_name(self):
        # tests overriding a method of the same service
        write_file("foo.thrift", textwrap.dedent("""\
            service MyS {
                void meh(),
                void meh(),
            }
        """))
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:3] Function MyS.meh redefines MyS.meh\n",
        )

    def test_duplicate_method_name_base_base(self):
        # tests overriding a method of the parent and the grandparent services
        write_file("foo.thrift", textwrap.dedent("""\
            service MySBB {
                void lol(),
            }
        """))
        write_file("bar.thrift", textwrap.dedent("""\
            include "foo.thrift"
            service MySB extends foo.MySBB {
                void meh(),
            }
        """))
        write_file("baz.thrift", textwrap.dedent("""\
            include "bar.thrift"
            service MyS extends bar.MySB {
                void lol(),
                void meh(),
            }
        """))
        ret, out, err = self.run_thrift("baz.thrift")
        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:baz.thrift:3] Function MyS.lol redefines service "
            "foo.MySBB.lol\n"
            "[FAILURE:baz.thrift:4] Function MyS.meh redefines service "
            "bar.MySB.meh\n",
        )

    def test_duplicate_enum_value_name(self):
        write_file("foo.thrift", textwrap.dedent("""\
            enum Foo {
                Bar = 1,
                Bar = 2,
            }
        """))
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:3] Redefinition of value Bar in enum Foo\n"
        )

    def test_duplicate_enum_value(self):
        write_file("foo.thrift", textwrap.dedent("""\
            enum Foo {
                Bar = 1,
                Baz = 1,
            }
        """))
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:3] "
            "Duplicate value Baz=1 with value Bar in enum Foo.\n"
        )

    def test_unset_enum_value(self):
        write_file("foo.thrift", textwrap.dedent("""\
            enum Foo {
                Bar,
                Baz,
            }
        """))
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:2] Unset enum value Bar in enum Foo. "
            "Add an explicit value to suppress this error\n"
            "[FAILURE:foo.thrift:3] Unset enum value Baz in enum Foo. "
            "Add an explicit value to suppress this error\n"
        )

    def test_circular_include_dependencies(self):
        # tests overriding a method of the parent and the grandparent services
        write_file("foo.thrift", textwrap.dedent("""\
            include "bar.thrift"
            service MySBB {
                void lol(),
            }
        """))
        write_file("bar.thrift", textwrap.dedent("""\
            include "foo.thrift"
            service MySB extends foo.MySBB {
                void meh(),
            }
        """))
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:bar.thrift:5] Circular dependency found: file "
            "foo.thrift is already parsed.\n"
        )

    def test_nonexistent_type(self):
        write_file("foo.thrift", textwrap.dedent("""\
            struct S {
                1: Random.Type field
            }
        """))

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:4] Type \"Random.Type\" not defined.\n"
        )
