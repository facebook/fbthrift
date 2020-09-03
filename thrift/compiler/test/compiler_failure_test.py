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

    def run_thrift(self, *args, gen="mstch_cpp2"):
        argsx = [thrift, "--gen", gen]
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

    def test_field_names_uniqueness(self):
        write_file("foo.thrift", textwrap.dedent("""\
            struct Foo {
              1: i32 a;
              2: i32 b;
              3: i64 a;
            }
        """))

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            '[FAILURE:foo.thrift:4] Field `a` is not unique in struct `Foo`\n',
        )

    def test_mixin_field_names_uniqueness(self):
        write_file("foo.thrift", textwrap.dedent("""\
            struct A { 1: i32 i }
            struct B { 2: i64 i }
            struct C {
              1: A a (cpp.mixin);
              2: B b (cpp.mixin);
            }
        """))

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            '[FAILURE:foo.thrift:5] Field `a.i` and `b.i` can not have same name in struct `C`\n',
        )

        write_file("bar.thrift", textwrap.dedent("""\
            struct A { 1: i32 i }
            struct B {
              1: A a (cpp.mixin);
              2: i64 i;
            }
        """))

        ret, out, err = self.run_thrift("bar.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            '[FAILURE:bar.thrift:3] Field `B.i` and `a.i` can not have same name in struct `B`\n',
        )

    def test_mixin_nonstruct_members(self):
        write_file("foo.thrift", textwrap.dedent("""\
            struct A {
              1: i32 i (cpp.mixin);
            }
        """))

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            '[FAILURE:foo.thrift:2] Mixin field `i` is not a struct but i32\n',
        )

    def test_mixin_in_union(self):
        write_file("foo.thrift", textwrap.dedent("""\
            struct A { 1: i32 i }
            union B {
              1: A a (cpp.mixin);
            }
        """))

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            '[FAILURE:foo.thrift:2] Union `B` can not have mixin field `a`\n',
        )

    def test_union_as_mixin(self):
        write_file("foo.thrift", textwrap.dedent("""\
            union A { 1: i32 i }
            struct B {
              1: A a (cpp.mixin);
            }
        """))

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            '[FAILURE:foo.thrift:3] Mixin field `a` is not a struct but union\n',
        )

    def test_mixin_with_cpp_ref(self):
        write_file("foo.thrift", textwrap.dedent("""\
            struct A { 1: i32 i }
            struct B {
              1: A a (cpp.ref = "true", cpp.mixin);
            }
        """))

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err, textwrap.dedent('''\
            [FAILURE:foo.thrift:3] Mixin field `a` can not have annotation (cpp.ref = true)
            [WARNING:foo.thrift:3] cpp.ref field must be optional if it is recursive
            [WARNING:foo.thrift:3] cpp.ref field must be optional if it is recursive
        ''')
        )

    def test_cpp_coroutine_mixin_stack_arguments(self):
        write_file("foo.thrift", textwrap.dedent("""\
            service SumService {
                i32 sum(1: i32 num1, 2: i32 num2) (cpp.coroutine);
            }
        """))

        GEN = "mstch_cpp2:stack_arguments"

        ret, out, err = self.run_thrift("foo.thrift", gen=GEN)

        self.assertEqual(ret, 0)

        write_file("foo.thrift", textwrap.dedent("""\
            service SumService {
                i32 sum(1: i32 num1, 2: string str) (cpp.coroutine);
            }
        """))

        ret, out, err = self.run_thrift("foo.thrift", gen=GEN)

        self.assertEqual(ret, 1)
        self.assertEqual(err, "[FAILURE:foo.thrift:2] SumService:sum use of "
            "cpp.coroutine and stack_arguments together is disallowed\n")

        write_file("foo.thrift", textwrap.dedent("""\
            service SumService {
                i32 sum(1: i32 num1, 2: i32 num2);
            }
        """))

        ret, out, err = self.run_thrift("foo.thrift", gen=GEN)

        self.assertEqual(ret, 0)

    def test_optional_mixin_field(self):
        write_file("foo.thrift", textwrap.dedent("""\
            struct A { 1: i32 i }
            struct B {
              1: optional A a (cpp.mixin);
            }
        """))

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err, textwrap.dedent(
                '[FAILURE:foo.thrift:3] Mixin field `a` can not be optional\n'
            )
        )

    def test_structured_annotations_uniqueness(self):
        write_file("foo.thrift", textwrap.dedent("""\
            struct Foo {
                1: i32 count;
            }

            @Foo{count=1}
            @Foo{count=2}
            struct Annotated {
                1: string name;
            }
        """))

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(err, "[FAILURE:foo.thrift:6] Duplicate structured "
            "annotation `struct foo.Foo` in the `struct foo.Annotated`.\n")

    def test_structured_annotations_type_resolved(self):
        write_file("foo.thrift", textwrap.dedent("""\
            struct Annotation {
                1: i32 count;
                2: TooForward forward;
            }

            @Annotation{count=1, forward=TooForward{name="abc"}}
            struct Annotated {
                1: string name;
            }

            struct TooForward {
                1: string name;
            }
        """))

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(err, "[FAILURE:foo.thrift:6] The type 'TooForward' "
            "is not defined yet. Types must be defined before the usage in "
            "constant values.\n")

    def test_too_many_splits(self):
        write_file("foo.thrift", textwrap.dedent("""\
            struct Foo { 1: i32 field }
            struct Bar { 1: i32 field }
            exception Baz { 1: i32 field }
            enum E { f = 0 }
            service SumService {
                i32 sum(1: i32 num1, 2: i32 num2);
            }
        """))

        ret, out, err = self.run_thrift("foo.thrift", gen='mstch_cpp2:types_cpp_splits=4')
        self.assertEqual(ret, 0)

        ret, out, err = self.run_thrift("foo.thrift", gen='mstch_cpp2:types_cpp_splits=5')

        self.assertEqual(ret, 1)
        self.assertEqual(err, "[FAILURE:foo.thrift] types_cpp_splits=5 is misconfigured: "
                "it can not be greater than number of object, which is 4.\n")
