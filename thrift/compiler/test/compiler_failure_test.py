# Copyright (c) Meta Platforms, Inc. and affiliates.
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

from __future__ import absolute_import, division, print_function, unicode_literals

import os
import shutil
import subprocess
import sys
import tempfile
import textwrap
import unittest
from contextlib import ExitStack
from importlib import resources

file_manager = ExitStack()
thrift = str(
    file_manager.enter_context(resources.path(__package__, "thrift")).absolute()
)


def write_file(path, content):
    if d := os.path.dirname(path):
        os.makedirs(d)
    with open(path, "w") as f:
        f.write(content)


class CompilerFailureTest(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, tmp, True)
        self.tmp = tmp
        self.addCleanup(os.chdir, os.getcwd())
        os.chdir(self.tmp)
        os.makedirs("thrift")
        with resources.path(__package__, "annotation_files") as files:
            shutil.copytree(files, "thrift/annotation")
        self.maxDiff = None

    def run_thrift(self, *args, gen="mstch_cpp2"):
        # Annotation files are copied from `thrift/annotation`
        argsx = [thrift, "--gen", gen, "-I", self.tmp]
        argsx.extend(args)
        pipe = subprocess.PIPE
        p = subprocess.Popen(argsx, stdout=pipe, stderr=pipe)
        out, err = p.communicate()
        out = out.decode(sys.getdefaultencoding())
        err = err.decode(sys.getdefaultencoding())
        err = err.replace("{}/".format(self.tmp), "")
        return p.returncode, out, err

    def test_double_package(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                package "test.dev/test"
                package "test.dev/test"
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:2] Package already specified.\n",
        )
        self.assertEqual(ret, 1)

    def test_zero_as_field_id(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct Foo {
                    0: i32 field;
                    1: list<i32> other;
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(
            err,
            "[WARNING:foo.thrift:2] Nonpositive field id (0) differs from what is auto-assigned by thrift. The id must be positive or -1.\n"
            "[WARNING:foo.thrift:2] No field id specified for `field`, resulting protocol may have conflicts or not be backwards compatible!\n",
        )
        self.assertEqual(ret, 0)

        ret, out, err = self.run_thrift("--allow-neg-keys", "foo.thrift")
        self.assertEqual(
            err,
            "[WARNING:foo.thrift:2] Nonpositive field id (0) differs from what would be auto-assigned by thrift (-1).\n"
            "[FAILURE:foo.thrift:2] Zero value (0) not allowed as a field id for `field`\n",
        )
        self.assertEqual(ret, 1)

    def test_zero_as_field_id_allowed(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct Foo {
                    0: i32 field (cpp.deprecated_allow_zero_as_field_id);
                    1: list<i32> other;
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(
            err,
            "[WARNING:foo.thrift:2] Nonpositive field id (0) differs from what is auto-assigned by thrift. The id must be positive or -1.\n"
            "[WARNING:foo.thrift:2] No field id specified for `field`, resulting protocol may have conflicts or not be backwards compatible!\n",
        )
        self.assertEqual(ret, 0)

        ret, out, err = self.run_thrift("--allow-neg-keys", "foo.thrift")
        self.assertEqual(
            err,
            "[WARNING:foo.thrift:2] Nonpositive field id (0) differs from what would be auto-assigned by thrift (-1).\n",
        )
        self.assertEqual(ret, 0)

    def test_negative_field_ids(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct Foo {
                    i32 f1;  // auto id = -1
                    -2: i32 f2; // auto and manual id = -2
                    -32: i32 f3; // min value.
                    -33: i32 f4; // min value - 1.
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(
            err,
            "[WARNING:foo.thrift:3] Nonpositive value (-2) not allowed as a field id.\n"
            "[WARNING:foo.thrift:4] Nonpositive field id (-32) differs from what is auto-assigned by thrift. The id must be positive or -3.\n"
            "[WARNING:foo.thrift:5] Nonpositive field id (-33) differs from what is auto-assigned by thrift. The id must be positive or -4.\n"
            "[WARNING:foo.thrift:2] No field id specified for `f1`, resulting protocol may have conflicts or not be backwards compatible!\n"
            "[WARNING:foo.thrift:4] No field id specified for `f3`, resulting protocol may have conflicts or not be backwards compatible!\n"
            "[WARNING:foo.thrift:5] No field id specified for `f4`, resulting protocol may have conflicts or not be backwards compatible!\n",
        )
        self.assertEqual(ret, 0)

        ret, out, err = self.run_thrift("--allow-neg-keys", "foo.thrift")
        self.assertEqual(
            err,
            "[WARNING:foo.thrift:4] Nonpositive field id (-32) differs from what would be auto-assigned by thrift (-3).\n"
            "[WARNING:foo.thrift:2] No field id specified for `f1`, resulting protocol may have conflicts or not be backwards compatible!\n"
            "[FAILURE:foo.thrift:5] Reserved field id (-33) cannot be used for `f4`.\n",
        )
        self.assertEqual(ret, 1)

    def test_exhausted_field_ids(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct Foo {
                    -32: i32 f1; // min value.
                    i32 f2; // auto id = -2 or min value - 1.
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(
            err,
            "[WARNING:foo.thrift:2] Nonpositive field id (-32) differs from what is auto-assigned by thrift. The id must be positive or -1.\n"
            "[WARNING:foo.thrift:2] No field id specified for `f1`, resulting protocol may have conflicts or not be backwards compatible!\n"
            "[WARNING:foo.thrift:3] No field id specified for `f2`, resulting protocol may have conflicts or not be backwards compatible!\n",
        )
        self.assertEqual(ret, 0)

        ret, out, err = self.run_thrift("--allow-neg-keys", "foo.thrift")
        self.assertEqual(
            err,
            "[WARNING:foo.thrift:2] Nonpositive field id (-32) differs from what would be auto-assigned by thrift (-1).\n"
            "[FAILURE:foo.thrift:3] Cannot allocate an id for `f2`. Automatic field ids are exhausted.\n"
            "[WARNING:foo.thrift:3] No field id specified for `f2`, resulting protocol may have conflicts or not be backwards compatible!\n"
            "[FAILURE:foo.thrift:3] Reserved field id (-33) cannot be used for `f2`.\n",
        )
        self.assertEqual(ret, 1)

    def test_too_many_fields(self):
        id_count = 33
        lines = ["struct Foo {"] + [f"i32 field_{i}" for i in range(id_count)] + ["}"]
        write_file("foo.thrift", "\n".join(lines))

        lines = [
            f"[WARNING:foo.thrift:{i + 2}] No field id specified for `field_{i}`, "
            "resulting protocol may have conflicts or not be backwards compatible!\n"
            for i in range(id_count)
        ]

        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:34] Cannot allocate an id for `field_32`. Automatic field ids are exhausted.\n"
            + "".join(lines)
            + "[FAILURE:foo.thrift:34] Reserved field id (-33) cannot be used for `field_32`.\n",
        )

    def test_out_of_range_field_ids(self):
        write_file(
            "overflow.thrift",
            textwrap.dedent(
                """\
                struct Foo {
                    -32768: i32 f1;
                    32767: i32 f2;
                    32768: i32 f3;
                }
                """
            ),
        )
        write_file(
            "underflow.thrift",
            textwrap.dedent(
                """\
                struct Foo {
                    -32768: i32 f4;
                    32767: i32 f5;
                    -32769: i32 f6;
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("overflow.thrift")
        self.assertEqual(
            err,
            "[FAILURE:overflow.thrift:4] Integer constant 32768 outside the range of field ids ([-32768, 32767]).\n"
            "[WARNING:overflow.thrift:2] Nonpositive field id (-32768) differs from what is auto-assigned by thrift. The id must be positive or -1.\n"
            "[WARNING:overflow.thrift:4] Nonpositive field id (-32768) differs from what is auto-assigned by thrift. The id must be positive or -2.\n"
            "[WARNING:overflow.thrift:2] No field id specified for `f1`, resulting protocol may have conflicts or not be backwards compatible!\n"
            "[WARNING:overflow.thrift:4] No field id specified for `f3`, resulting protocol may have conflicts or not be backwards compatible!\n",
        )
        self.assertEqual(ret, 1)
        ret, out, err = self.run_thrift("underflow.thrift")
        self.assertEqual(
            err,
            "[FAILURE:underflow.thrift:4] Integer constant -32769 outside the range of field ids ([-32768, 32767]).\n"
            "[WARNING:underflow.thrift:2] Nonpositive field id (-32768) differs from what is auto-assigned by thrift. The id must be positive or -1.\n"
            '[FAILURE:underflow.thrift:4] Field identifier 32767 for "f6" has already been used.\n'
            "[WARNING:underflow.thrift:2] No field id specified for `f4`, resulting protocol may have conflicts or not be backwards compatible!\n",
        )
        self.assertEqual(ret, 1)

    def test_oneway_return_type(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                service MyService {
                    oneway void foo();
                    oneway string bar();
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        # TODO(afuller): Report a diagnostic instead.
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:3] Oneway methods must have void return type: bar\n",
        )
        self.assertEqual(ret, 1)

    def test_oneway_exception(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                exception A {}

                service MyService {
                    oneway void foo();
                    oneway void baz() throws (1: A ex);
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        # TODO(afuller): Report a diagnostic instead.
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:5] Oneway methods can't throw exceptions: baz\n",
        )
        self.assertEqual(ret, 1)

    def test_enum_wrong_default_value(self):
        # tests initializing enum with default value of wrong type
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                enum Color {
                    RED = 1,
                    GREEN = 2,
                    BLUE = 3,
                }

                struct MyS {
                    1: Color color = -1
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(ret, 0)
        self.assertEqual(
            err,
            "[WARNING:foo.thrift:8] type error: const `color` was declared as enum `Color` with "
            "a value not of that enum.\n",
        )

    def test_enum_overflow(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                    enum Foo {
                        Bar = 2147483647
                        Baz = 2147483648
                    }
                    """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:3] Integer constant 2147483648 outside the range of enum values ([-2147483648, 2147483647]).\n",
        )

    def test_enum_underflow(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                    enum Foo {
                        Bar = -2147483648
                        Baz = -2147483649
                    }
                    """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:3] Integer constant -2147483649 outside the range of enum values ([-2147483648, 2147483647]).\n",
        )

    def assertConstError(self, type, value, expected):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                f"""\
                    const {type} overflowInt = {value};
                    """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(ret, 1, err)
        self.assertEqual(err, expected)

    def test_integer_overflow(self):
        self.assertConstError(
            "i64",
            "9223372036854775808",  # max int64 + 1
            "[FAILURE:foo.thrift:1] integer constant 9223372036854775808 is too large\n"
            "[WARNING:foo.thrift:1] 64-bit constant -9223372036854775808 may not work in all languages\n",
        )
        self.assertConstError(
            "i64",
            "18446744073709551615",  # max uint64
            "[FAILURE:foo.thrift:1] integer constant 18446744073709551615 is too large\n",
        )
        self.assertConstError(
            "i64",
            "18446744073709551616",  # max uint64 + 1
            "[FAILURE:foo.thrift:1] integer constant 18446744073709551616 is too large\n",
        )

    def test_double_overflow(self):
        for value in [
            "1.7976931348623159e+308",
            "1.7976931348623158e+309",
        ]:
            self.assertConstError(
                "double",
                f"{value}",
                f"[FAILURE:foo.thrift:1] floating-point constant {value} is out of range\n",
            )
            self.assertConstError(
                "double",
                f"-{value}",
                f"[FAILURE:foo.thrift:1] floating-point constant {value} is out of range\n",
            )

    def test_integer_underflow(self):
        self.assertConstError(
            "i64",
            "-9223372036854775809",
            "[FAILURE:foo.thrift:1] integer constant -9223372036854775809 is too small\n"
            "[WARNING:foo.thrift:1] 64-bit constant 9223372036854775807 may not work in all languages\n",
        )

    def test_double_underflow(self):
        for value in [
            "4.9406564584124654e-325",
            "1e-324",
        ]:
            self.assertConstError(
                "double",
                f"{value}",
                f"[FAILURE:foo.thrift:1] magnitude of floating-point constant {value} is too small\n",
            )
            self.assertConstError(
                "double",
                f"-{value}",
                f"[FAILURE:foo.thrift:1] magnitude of floating-point constant {value} is too small\n",
            )

    def test_const_wrong_type(self):
        # tests initializing const with value of wrong type
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                const i32 wrongInt = "stringVal"
                const set<string> wrongSet = {1: 2}
                const map<i32, i32> wrongMap = [1,32,3];
                const map<i32, i32> wierdMap = [];
                const set<i32> wierdSet = {};
                const list<i32> wierdList = {};
                const list<string> badValList = [1]
                const set<string> badValSet = [2]
                const map<string, i32> badValMap = {1: "str"}
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:1] type error: const `wrongInt` was declared as i32.\n"
            "[WARNING:foo.thrift:2] type error: const `wrongSet` was declared as set. This will become an error in future versions of thrift.\n"
            "[WARNING:foo.thrift:3] type error: const `wrongMap` was declared as map. This will become an error in future versions of thrift.\n"
            "[WARNING:foo.thrift:4] type error: map `wierdMap` initialized with empty list.\n"
            "[WARNING:foo.thrift:5] type error: set `wierdSet` initialized with empty map.\n"
            "[WARNING:foo.thrift:6] type error: list `wierdList` initialized with empty map.\n"
            "[FAILURE:foo.thrift:7] type error: const `badValList<elem>` was declared as string.\n"
            "[FAILURE:foo.thrift:8] type error: const `badValSet<elem>` was declared as string.\n"
            "[FAILURE:foo.thrift:9] type error: const `badValMap<key>` was declared as string.\n"
            "[FAILURE:foo.thrift:9] type error: const `badValMap<val>` was declared as i32.\n",
        )

    def test_struct_fields_wrong_type(self):
        # tests initializing structured annotation value of wrong type
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct Annot {
                    1: i32 val
                    2: list<string> otherVal
                }

                @Annot{val="hi", otherVal=5}
                struct BadFields {
                    1: i32 badInt = "str"
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:6] type error: const `.val` was declared as i32.\n"
            "[WARNING:foo.thrift:6] type error: const `.otherVal` was declared as list. This will become an error in future versions of thrift.\n"
            "[FAILURE:foo.thrift:8] type error: const `badInt` was declared as i32.\n",
        )

    def test_duplicate_method_name(self):
        # tests overriding a method of the same service
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                service MyS {
                    void meh(),
                    void meh(),
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(ret, 1)
        self.assertEqual(
            err, "[FAILURE:foo.thrift:3] Function `meh` is already defined for `MyS`.\n"
        )

    def test_duplicate_method_name_base_base(self):
        # tests overriding a method of the parent and the grandparent services
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                service MySBB {
                    void lol(),
                }
                """
            ),
        )
        write_file(
            "bar.thrift",
            textwrap.dedent(
                """\
                include "foo.thrift"
                service MySB extends foo.MySBB {
                    void meh(),
                }
                """
            ),
        )
        write_file(
            "baz.thrift",
            textwrap.dedent(
                """\
                include "bar.thrift"
                service MyS extends bar.MySB {
                    void lol(),
                    void meh(),
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("baz.thrift")
        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:baz.thrift:3] Function `MyS.lol` redefines `service foo.MySBB.lol`.\n"
            "[FAILURE:baz.thrift:4] Function `MyS.meh` redefines `service bar.MySB.meh`.\n",
        )

    def test_duplicate_enum_value_name(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                enum Foo {
                    Bar = 1,
                    Bar = 2,
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:3] Enum value `Bar` is already defined for `Foo`.\n",
        )

    def test_duplicate_enum_value(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                enum Foo {
                    Bar = 1,
                    Baz = 1,
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:3] Duplicate value `Baz=1` with value `Bar` in enum `Foo`.\n",
        )

    def test_unset_enum_value(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                enum Foo {
                    Foo = 1,
                    Bar,
                    Baz,
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:3] The enum value, `Bar`, must have an explicitly assigned value.\n"
            "[FAILURE:foo.thrift:4] The enum value, `Baz`, must have an explicitly assigned value.\n",
        )

    def test_circular_include_dependencies(self):
        # tests overriding a method of the parent and the grandparent services
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                include "bar.thrift"
                service MySBB {
                    void lol(),
                }
                """
            ),
        )
        write_file(
            "bar.thrift",
            textwrap.dedent(
                """\
                include "foo.thrift"
                service MySB extends foo.MySBB {
                    void meh(),
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:bar.thrift:5] Circular dependency found: "
            "file `foo.thrift` is already parsed.\n",
        )

    def test_nonexistent_type(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct S {
                    1: Random.Type field
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err, "[FAILURE:foo.thrift:2] Type `Random.Type` not defined.\n"
        )

    def test_field_names_uniqueness(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct Foo {
                    1: i32 a;
                    2: i32 b;
                    3: i64 a;
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err, "[FAILURE:foo.thrift:4] Field `a` is already defined for `Foo`.\n"
        )

    def test_mixin_field_names_uniqueness(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct A { 1: i32 i }
                struct B { 2: i64 i }
                struct C {
                    1: A a (cpp.mixin);
                    2: B b (cpp.mixin);
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:5] Field `B.i` and `A.i` can not have same name in `C`.\n",
        )

        write_file(
            "bar.thrift",
            textwrap.dedent(
                """\
                struct A { 1: i32 i }

                struct C {
                    1: A a (cpp.mixin);
                    2: i64 i;
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("bar.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:bar.thrift:5] Field `C.i` and `A.i` can not have same name in `C`.\n",
        )

    def test_struct_optional_refs(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct A {
                    1: A rec (cpp.ref);
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 0)
        self.assertEqual(
            err,
            "[WARNING:foo.thrift:2] `cpp.ref` field `rec` must be optional if it is recursive.\n"
            "[WARNING:foo.thrift:2] cpp.ref, cpp2.ref are deprecated. Please use @thrift.Box annotation instead in `rec`.\n",
        )

    def test_structured_ref(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                include "thrift/annotation/cpp.thrift"

                struct Foo {
                    1: optional Foo field1 (cpp.ref);

                    @cpp.Ref{type = cpp.RefType.Unique}
                    2: optional Foo field2;

                    @cpp.Ref{type = cpp.RefType.Unique}
                    3: optional Foo field3 (cpp.ref);

                    @cpp.Ref{type = cpp.RefType.Unique}
                    @cpp.Ref{type = cpp.RefType.Unique}
                    4: optional Foo field4;
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            "\n" + err,
            textwrap.dedent(
                """
                [WARNING:foo.thrift:4] cpp.ref, cpp2.ref are deprecated. Please use @thrift.Box annotation instead in `field1`.
                [WARNING:foo.thrift:6] @cpp.Ref{type = cpp.RefType.Unique} is deprecated. Please use @thrift.Box annotation instead in `field2`.
                [FAILURE:foo.thrift:10] The @cpp.Ref annotation cannot be combined with the `cpp.ref` or `cpp.ref_type` annotations. Remove one of the annotations from `field3`.
                [WARNING:foo.thrift:9] cpp.ref, cpp2.ref are deprecated. Please use @thrift.Box annotation instead in `field3`.
                [WARNING:foo.thrift:9] @cpp.Ref{type = cpp.RefType.Unique} is deprecated. Please use @thrift.Box annotation instead in `field3`.
                [WARNING:foo.thrift:12] @cpp.Ref{type = cpp.RefType.Unique} is deprecated. Please use @thrift.Box annotation instead in `field4`.
                [FAILURE:foo.thrift:13] Structured annotation `Ref` is already defined for `field4`.
            """
            ),
        )

    def test_adapter(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                include "thrift/annotation/cpp.thrift"

                typedef i64 MyI64 (cpp.adapter="MyAdapter")

                struct MyStruct {
                    @cpp.Adapter{name="MyAdapter"}
                    1: MyI64 my_field;
                    @cpp.Adapter{}
                    2: i64 my_field2;
                    @cpp.Adapter{name="MyAdapter"}
                    3: optional i64 my_field3 (cpp.ref);
                    @cpp.Adapter{name="MyAdapter"}
                    4: optional i64 my_field4 (cpp.ref_type = "unique");
                    @cpp.Adapter{name="MyAdapter"}
                    @cpp.Ref{type = cpp.RefType.Unique}
                    5: optional i64 my_field5;
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:7] `@cpp.Adapter` cannot be combined with "
            "`cpp_adapter` in `my_field`.\n"
            "[FAILURE:foo.thrift:9] key `name` not found.\n"
            "[FAILURE:foo.thrift:11] cpp.ref, cpp2.ref are deprecated. "
            "Please use @thrift.Box annotation instead in `my_field3` with @cpp.Adapter.\n"
            "[FAILURE:foo.thrift:13] cpp.ref_type = `unique`, cpp2.ref_type = `unique` "
            "are deprecated. Please use @thrift.Box annotation instead in `my_field4` with @cpp.Adapter.\n"
            "[FAILURE:foo.thrift:16] @cpp.Ref{type = cpp.RefType.Unique} is deprecated. "
            "Please use @thrift.Box annotation instead in `my_field5` with @cpp.Adapter.\n",
        )

    def test_typedef_adapter(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                include "thrift/annotation/cpp.thrift"

                @cpp.Adapter{}
                typedef i32 MyI32

                @cpp.Adapter{name="MyAdapter"}
                typedef i64 MyI64

                @cpp.Adapter{name="MyAdapter"}
                typedef MyI64 DoubleMyI64

                struct MyStruct {
                    1: MyI64 my_field;
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:4] key `name` not found.\n"
            "[FAILURE:foo.thrift:10] The @cpp.Adapter annotation cannot be annotated more "
            "than once in all typedef levels in `DoubleMyI64`.\n",
        )

    def test_mixin_nonstruct_members(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct A {
                    1: i32 i (cpp.mixin);
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:2] Mixin field `i` type must be a struct or union. Found `i32`.\n",
        )

    def test_mixin_in_union(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct A { 1: i32 i }
                union B {
                    1: A a (cpp.mixin);
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err, "[FAILURE:foo.thrift:3] Union `B` cannot contain mixin field `a`.\n"
        )

    def test_mixin_with_cpp_ref(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct A { 1: i32 i }
                struct B {
                    1: A a (cpp.ref = "true", cpp.mixin);
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            textwrap.dedent(
                """\
                [FAILURE:foo.thrift:3] Mixin field `a` can not be a ref in cpp.
                [WARNING:foo.thrift:3] `cpp.ref` field `a` must be optional if it is recursive.
                [WARNING:foo.thrift:3] cpp.ref, cpp2.ref are deprecated. Please use @thrift.Box annotation instead in `a`.
                """
            ),
        )

    def test_bitpack_with_tablebased_seriliazation(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                include "thrift/annotation/cpp.thrift"
                struct A { 1: i32 i }
                @cpp.PackIsset
                struct D { 1: i32 i }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift", gen="mstch_cpp2:json,tablebased")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            textwrap.dedent(
                """\
                [FAILURE:foo.thrift:4] Tablebased serialization is incompatible with isset bitpacking for struct `D`
                """
            ),
        )

    def test_cpp_coroutine_mixin_stack_arguments(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                service SumService {
                    i32 sum(1: i32 num1, 2: i32 num2) (cpp.coroutine);
                }
                """
            ),
        )

        GEN = "mstch_cpp2:stack_arguments"

        ret, out, err = self.run_thrift("foo.thrift", gen=GEN)

        self.assertEqual(ret, 0)

        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                service SumService {
                    i32 sum(1: i32 num1, 2: string str) (cpp.coroutine);
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift", gen=GEN)

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:2] `SumService.sum` use of "
            "cpp.coroutine and stack_arguments together is disallowed.\n",
        )

        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                service SumService {
                    i32 sum(1: i32 num1, 2: i32 num2);
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift", gen=GEN)

        self.assertEqual(ret, 0)

    def test_optional_mixin_field(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct A { 1: i32 i }
                struct B {
                    1: optional A a (cpp.mixin);
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            textwrap.dedent(
                "[FAILURE:foo.thrift:3] Mixin field `a` cannot be optional.\n"
            ),
        )

    def test_structured_annotations_uniqueness(self):
        write_file(
            "other/foo.thrift",
            textwrap.dedent(
                """\
                struct Foo {}
                """
            ),
        )
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                include "other/foo.thrift"
                struct Foo {
                    1: i32 count;
                }

                @foo.Foo
                @Foo{count=1}
                @Foo{count=2}
                typedef i32 Annotated
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            # TODO(afuller): Fix t_scope to not include the locally defined Foo as
            # `foo.Foo`, which override the included foo.Foo definition.
            "[FAILURE:foo.thrift:7] Structured annotation `Foo` is already defined for `Annotated`.\n"
            "[FAILURE:foo.thrift:8] Structured annotation `Foo` is already defined for `Annotated`.\n",
        )

    def test_structured_annotations_type_resolved(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
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
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:6] The type 'TooForward' "
            "is not defined yet. Types must be defined before the usage in "
            "constant values.\n",
        )

    def test_too_many_splits(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct Foo { 1: i32 field }
                struct Bar { 1: i32 field }
                exception Baz { 1: i32 field }
                enum E { f = 0 }
                service SumService {
                    i32 sum(1: i32 num1, 2: i32 num2);
                }
                """
            ),
        )

        ret, out, err = self.run_thrift(
            "foo.thrift", gen="mstch_cpp2:types_cpp_splits=4"
        )
        self.assertEqual(ret, 0)

        ret, out, err = self.run_thrift(
            "foo.thrift", gen="mstch_cpp2:types_cpp_splits=5"
        )

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift] `types_cpp_splits=5` is misconfigured: "
            "it can not be greater than number of object, which is 4.\n",
        )

        ret, out, err = self.run_thrift(
            "foo.thrift", gen="mstch_cpp2:types_cpp_splits=3a"
        )

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift] Invalid types_cpp_splits value: `3a`\n",
        )

    def test_invalid_splits(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct Foo { 1: i32 field }
                """
            ),
        )

        ret, out, err = self.run_thrift(
            "foo.thrift", gen="mstch_cpp2:types_cpp_splits=1a"
        )

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift] Invalid types_cpp_splits value: `1a`\n",
        )

    def test_too_many_client_splits(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                service MyService1 {
                    i32 func1(1: i32 num);
                    i32 func2(1: i32 num);
                    i32 func3(1: i32 num);
                }
                service MyService2 {
                    i32 func1(1: i32 num);
                    i32 func2(1: i32 num);
                }
                """
            ),
        )

        ret, out, err = self.run_thrift(
            "foo.thrift", gen="mstch_cpp2:client_cpp_splits={MyService1:3,MyService2:2}"
        )
        self.assertEqual(ret, 0)

        ret, out, err = self.run_thrift(
            "foo.thrift", gen="mstch_cpp2:client_cpp_splits={MyService1:3,MyService2:3}"
        )
        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:6] `client_cpp_splits=3` (For service MyService2) is misconfigured: it can not be greater than number of functions, which is 2.\n",
        )

    def test_invalid_client_splits(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                service MyService1 {
                    i32 func1(1: i32 num);
                    i32 func2(1: i32 num);
                    i32 func3(1: i32 num);
                }
                service MyService2 {
                    i32 func1(1: i32 num);
                    i32 func2(1: i32 num);
                }
                """
            ),
        )

        ret, out, err = self.run_thrift(
            "foo.thrift",
            gen="mstch_cpp2:client_cpp_splits={MyService1:3:1,MyService2:2}",
        )
        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift] Invalid pair `MyService1:3:1` in client_cpp_splits value: `MyService1:3:1,MyService2:2`\n",
        )

        ret, out, err = self.run_thrift(
            "foo.thrift", gen="mstch_cpp2:client_cpp_splits={MyService3:1}"
        )
        self.assertEqual(ret, 0)

    def test_lazy_field(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct A {
                    1: i32 field (cpp.experimental.lazy)
                }
                typedef double FP
                struct B {
                    1: FP field (cpp.experimental.lazy)
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            textwrap.dedent(
                "[FAILURE:foo.thrift:2] Integral field `field` can not be"
                " marked as lazy, since doing so won't bring any benefit.\n"
                "[FAILURE:foo.thrift:6] Floating point field `field` can not be"
                " marked as lazy, since doing so won't bring any benefit.\n"
            ),
        )

    def test_bad_throws(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct A {}

                service B {
                    void foo() throws (1: A ex)
                    stream<i32 throws (1: A ex)> bar()
                    sink<i32 throws (1: A ex),
                         i32 throws (1: A ex)> baz()
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            textwrap.dedent(
                "[FAILURE:foo.thrift:4] Non-exception type, `A`, in throws.\n"
                "[FAILURE:foo.thrift:5] Non-exception type, `A`, in throws.\n"
                "[FAILURE:foo.thrift:6] Non-exception type, `A`, in throws.\n"
                "[FAILURE:foo.thrift:7] Non-exception type, `A`, in throws.\n"
            ),
        )

    def test_boxed_ref(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct A {
                    1: optional i64 field (cpp.ref, thrift.box)
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            textwrap.dedent(
                "[WARNING:foo.thrift:2] cpp.box and thrift.box are deprecated. Please use @thrift.Box annotation instead in `field`.\n"
                "[FAILURE:foo.thrift:2] The `cpp.box` annotation cannot be combined "
                "with the `cpp.ref` or `cpp.ref_type` annotations. Remove one of the "
                "annotations from `field`.\n"
            ),
        )

    def test_boxed_optional(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct A {
                    1: i64 field (thrift.box)
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            textwrap.dedent(
                "[WARNING:foo.thrift:2] cpp.box and thrift.box are deprecated. Please use @thrift.Box annotation instead in `field`.\n"
                "[FAILURE:foo.thrift:2] The `cpp.box` annotation can only be used with "
                "optional fields. Make sure `field` is optional.\n"
            ),
        )

    def test_nonexist_field_name(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct Foo {}
                typedef list<Foo> List
                const List l = [{"foo": "bar"}];
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            textwrap.dedent("[FAILURE:foo.thrift:3] field `foo` does not exist.\n"),
        )

    def test_annotation_scopes(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                include "thrift/annotation/scope.thrift"

                struct NotAnAnnot {}

                @scope.Struct
                struct StructAnnot{}
                @scope.Field
                struct FieldAnnot{}

                @scope.Struct
                @scope.Field
                struct StructOrFieldAnnot {}

                @scope.Enum
                struct EnumAnnot {}

                @NotAnAnnot
                @StructAnnot
                @FieldAnnot
                @StructOrFieldAnnot
                @EnumAnnot
                struct TestStruct {
                    @FieldAnnot
                    @StructAnnot
                    @StructOrFieldAnnot
                    1: bool test_field;
                }

                @EnumAnnot
                enum TestEnum { Foo = 0, Bar = 1 }
                """
            ),
        )

        ret, out, err = self.run_thrift("--strict", "foo.thrift")
        self.assertEqual(ret, 1)
        self.assertEqual(
            "\n" + err,
            textwrap.dedent(
                """
                [WARNING:foo.thrift:17] Using `NotAnAnnot` as an annotation, even though it has not been enabled for any annotation scope.
                [FAILURE:foo.thrift:20] `FieldAnnot` cannot annotate `TestStruct`
                [FAILURE:foo.thrift:22] `EnumAnnot` cannot annotate `TestStruct`
                [FAILURE:foo.thrift:25] `StructAnnot` cannot annotate `test_field`
                """
            ),
        )

        ret, out, err = self.run_thrift("--legacy-strict", "foo.thrift")
        self.assertEqual(ret, 1)
        self.assertEqual(
            "\n" + err,
            textwrap.dedent(
                """
                [WARNING:foo.thrift:17] Using `NotAnAnnot` as an annotation, even though it has not been enabled for any annotation scope.
                [FAILURE:foo.thrift:20] `FieldAnnot` cannot annotate `TestStruct`
                [FAILURE:foo.thrift:22] `EnumAnnot` cannot annotate `TestStruct`
                [FAILURE:foo.thrift:25] `StructAnnot` cannot annotate `test_field`
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(ret, 1)
        self.assertEqual(
            "\n" + err,
            textwrap.dedent(
                """
                [FAILURE:foo.thrift:20] `FieldAnnot` cannot annotate `TestStruct`
                [FAILURE:foo.thrift:22] `EnumAnnot` cannot annotate `TestStruct`
                [FAILURE:foo.thrift:25] `StructAnnot` cannot annotate `test_field`
                """
            ),
        )

    def test_lazy_struct_compatibility(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct Foo {
                    1: list<i32> field (cpp.experimental.lazy)
                } (cpp.methods = "")
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            textwrap.dedent(
                "[FAILURE:foo.thrift:1] cpp.methods is incompatible with lazy deserialization in struct `Foo`\n"
            ),
        )

    def test_duplicate_field_id(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct A {
                    1: i64 field1;
                    1: i64 field2;
                }

                struct B {
                    1: i64 field1;
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            textwrap.dedent(
                '[FAILURE:foo.thrift:3] Field identifier 1 for "field2" has already been used.\n'
            ),
        )

    def test_thrift_uri_uniqueness(self):
        write_file(
            "file1.thrift",
            textwrap.dedent(
                """
                struct Foo1 {
                } (thrift.uri = "facebook.com/thrift/annotation/Foo")
                """
            ),
        )

        write_file(
            "file2.thrift",
            textwrap.dedent(
                """
                struct Foo2 {
                } (thrift.uri = "facebook.com/thrift/annotation/Foo")
                struct Bar1 {
                } (thrift.uri = "facebook.com/thrift/annotation/Bar")
                """
            ),
        )

        write_file(
            "main.thrift",
            textwrap.dedent(
                """\
                include "file1.thrift"
                include "file2.thrift"
                struct Bar2 {
                } (thrift.uri = "facebook.com/thrift/annotation/Bar")
                struct Baz1 {
                } (thrift.uri = "facebook.com/thrift/annotation/Baz")
                struct Baz2 {
                } (thrift.uri = "facebook.com/thrift/annotation/Baz")
                """
            ),
        )

        ret, out, err = self.run_thrift("main.thrift")
        self.assertEqual(ret, 1)
        self.assertEqual(
            "\n" + err,
            textwrap.dedent(
                """
                [FAILURE:file2.thrift:2] Thrift URI `facebook.com/thrift/annotation/Foo` is already defined for `Foo2`.
                [FAILURE:main.thrift:3] Thrift URI `facebook.com/thrift/annotation/Bar` is already defined for `Bar2`.
                [FAILURE:main.thrift:7] Thrift URI `facebook.com/thrift/annotation/Baz` is already defined for `Baz2`.
                """
            ),
        )

    def test_unique_ref(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                include "thrift/annotation/cpp.thrift"

                struct Foo {
                    1: optional Foo field1 (cpp.ref);

                    2: optional Foo field2 (cpp2.ref);

                    @cpp.Ref{type = cpp.RefType.Unique}
                    3: optional Foo field3;

                    @cpp.Ref{type = cpp.RefType.Shared}
                    4: optional Foo field4;

                    @cpp.Ref{type = cpp.RefType.SharedMutable}
                    5: optional Foo field5;

                    6: optional Foo field6 (cpp.ref_type = "unique");

                    7: optional Foo field7 (cpp2.ref_type = "unique");

                    8: optional Foo field8 (cpp.ref_type = "shared");

                    9: optional Foo field9 (cpp2.ref_type = "shared");

                    10: optional Foo field10 (cpp.ref = "true");

                    11: optional Foo field11 (cpp2.ref = "true");

                    12: optional Foo field12;

                    13: optional Foo field13;

                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 0)
        self.assertEqual(
            "\n" + err,
            textwrap.dedent(
                """
                [WARNING:foo.thrift:4] cpp.ref, cpp2.ref are deprecated. Please use @thrift.Box annotation instead in `field1`.
                [WARNING:foo.thrift:6] cpp.ref, cpp2.ref are deprecated. Please use @thrift.Box annotation instead in `field2`.
                [WARNING:foo.thrift:8] @cpp.Ref{type = cpp.RefType.Unique} is deprecated. Please use @thrift.Box annotation instead in `field3`.
                [WARNING:foo.thrift:17] cpp.ref_type = `unique`, cpp2.ref_type = `unique` are deprecated. Please use @thrift.Box annotation instead in `field6`.
                [WARNING:foo.thrift:19] cpp.ref_type = `unique`, cpp2.ref_type = `unique` are deprecated. Please use @thrift.Box annotation instead in `field7`.
                [WARNING:foo.thrift:25] cpp.ref, cpp2.ref are deprecated. Please use @thrift.Box annotation instead in `field10`.
                [WARNING:foo.thrift:27] cpp.ref, cpp2.ref are deprecated. Please use @thrift.Box annotation instead in `field11`.
                """
            ),
        )

    def test_boxed(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct A {
                    1: optional i64 field (cpp.box)
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 0)
        self.assertEqual(
            "\n" + err,
            textwrap.dedent(
                """
                [WARNING:foo.thrift:2] cpp.box and thrift.box are deprecated. Please use @thrift.Box annotation instead in `field`.
                """
            ),
        )

    def test_terse_write_annotation(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                include "thrift/annotation/thrift.thrift"

                struct TerseFields {
                    @thrift.TerseWrite
                    1: i64 field1;
                    @thrift.TerseWrite
                    2: optional i64 field2;
                    @thrift.TerseWrite
                    3: required i64 field3;
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:7] `@thrift.TerseWrite` cannot be used with qualified fields. "
            "Remove `optional` qualifier from field `field2`.\n"
            "[FAILURE:foo.thrift:9] `@thrift.TerseWrite` cannot be used with qualified fields. "
            "Remove `required` qualifier from field `field3`.\n",
        )

    def test_interaction_nesting(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                interaction I {
                    void foo();
                }

                interaction J {
                    performs I;
                }

                interaction K {
                    I foo();
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:6] Nested interactions are forbidden.\n"
            "[FAILURE:foo.thrift:10] Nested interactions are forbidden: foo\n",
        )

    def test_interaction_oneway_factory(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                interaction I {
                    void foo();
                }

                service S {
                    oneway I foo();
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:6] Oneway methods must have void return type: foo\n",
        )

    def test_interaction_return_type_order(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                interaction I {
                    void foo();
                }

                service S {
                    i32, I foo();
                    i32, i32 bar();
                    i32, I, stream<i32> baz();
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:6] Only an interaction is allowed in this position\n"
            "[FAILURE:foo.thrift:7] Only an interaction is allowed in this position\n"
            "[FAILURE:foo.thrift:8] Only an interaction is allowed in this position\n"
            "[FAILURE:foo.thrift:6] Interactions are not allowed in this position\n"
            "[FAILURE:foo.thrift:8] Interactions are not allowed in this position\n",
        )

    def test_interaction_in_return_type(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                interaction I {
                    void foo();
                }

                service S {
                    I, I foo();
                    I, I, stream<i32> bar();
                    I, I, sink<i32, i32> baz();
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:6] Interactions are not allowed in this position\n"
            "[FAILURE:foo.thrift:7] Interactions are not allowed in this position\n"
            "[FAILURE:foo.thrift:8] Interactions are not allowed in this position\n",
        )

    # Time complexity of for_each_transitive_field should be O(1)
    def test_time_complexity_of_for_each_transitive_field(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct S_01 { 1: i32 i; }
                struct S_02 { 1: optional S_01 a (thrift.box); 2: optional S_01 b (thrift.box); }
                struct S_03 { 1: optional S_02 a (thrift.box); 2: optional S_02 b (thrift.box); }
                struct S_04 { 1: optional S_03 a (thrift.box); 2: optional S_03 b (thrift.box); }
                struct S_05 { 1: optional S_04 a (thrift.box); 2: optional S_04 b (thrift.box); }
                struct S_06 { 1: optional S_05 a (thrift.box); 2: optional S_05 b (thrift.box); }
                struct S_07 { 1: optional S_06 a (thrift.box); 2: optional S_06 b (thrift.box); }
                struct S_08 { 1: optional S_07 a (thrift.box); 2: optional S_07 b (thrift.box); }
                struct S_09 { 1: optional S_08 a (thrift.box); 2: optional S_08 b (thrift.box); }
                struct S_10 { 1: optional S_09 a (thrift.box); 2: optional S_09 b (thrift.box); }
                struct S_11 { 1: optional S_10 a (thrift.box); 2: optional S_10 b (thrift.box); }
                struct S_12 { 1: optional S_11 a (thrift.box); 2: optional S_11 b (thrift.box); }
                struct S_13 { 1: optional S_12 a (thrift.box); 2: optional S_12 b (thrift.box); }
                struct S_14 { 1: optional S_13 a (thrift.box); 2: optional S_13 b (thrift.box); }
                struct S_15 { 1: optional S_14 a (thrift.box); 2: optional S_14 b (thrift.box); }
                struct S_16 { 1: optional S_15 a (thrift.box); 2: optional S_15 b (thrift.box); }
                struct S_17 { 1: optional S_16 a (thrift.box); 2: optional S_16 b (thrift.box); }
                struct S_18 { 1: optional S_17 a (thrift.box); 2: optional S_17 b (thrift.box); }
                struct S_19 { 1: optional S_18 a (thrift.box); 2: optional S_18 b (thrift.box); }
                struct S_20 { 1: optional S_19 a (thrift.box); 2: optional S_19 b (thrift.box); }
                struct S_21 { 1: optional S_20 a (thrift.box); 2: optional S_20 b (thrift.box); }
                struct S_22 { 1: optional S_21 a (thrift.box); 2: optional S_21 b (thrift.box); }
                struct S_23 { 1: optional S_22 a (thrift.box); 2: optional S_22 b (thrift.box); }
                struct S_24 { 1: optional S_23 a (thrift.box); 2: optional S_23 b (thrift.box); }
                struct S_25 { 1: optional S_24 a (thrift.box); 2: optional S_24 b (thrift.box); }
                struct S_26 { 1: optional S_25 a (thrift.box); 2: optional S_25 b (thrift.box); }
                struct S_27 { 1: optional S_26 a (thrift.box); 2: optional S_26 b (thrift.box); }
                struct S_28 { 1: optional S_27 a (thrift.box); 2: optional S_27 b (thrift.box); }
                struct S_29 { 1: optional S_28 a (thrift.box); 2: optional S_28 b (thrift.box); }
                struct S_30 { 1: optional S_29 a (thrift.box); 2: optional S_29 b (thrift.box); }
                struct S_31 { 1: optional S_30 a (thrift.box); 2: optional S_30 b (thrift.box); }
                struct S_32 { 1: optional S_31 a (thrift.box); 2: optional S_31 b (thrift.box); }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 0)

    def test_inject_metadata_fields_annotation(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct Fields {
                    1: i64 field1;
                    2: optional i64 field2;
                    3: required i64 field3;
                }
                """
            ),
        )

        write_file(
            "bar.thrift",
            textwrap.dedent(
                """\
                include "thrift/annotation/internal.thrift"

                typedef i64 MyI64

                union UnionFields {
                    1: i64 field1;
                    2: i64 field2;
                }

                struct Fields {
                    1: i64 field1;
                    2: optional i64 field2;
                    3: required i64 field3;
                }

                @internal.InjectMetadataFields{type="foo.Fields"}
                struct Injected1 {}

                @internal.InjectMetadataFields
                struct Injected2 {}

                @internal.InjectMetadataFields{type="UnionFields"}
                struct Injected3 {}

                @internal.InjectMetadataFields{type="MyI64"}
                struct Injected4 {}

                @internal.InjectMetadataFields{type="Fields"}
                struct Injected5 {
                    1: i64 field1;
                }

                // If a field is explicitly assigned with field id 0,
                // the field id gets implicitly converted -1.
                struct BoundaryFields {
                    -1: i64 underflow;
                    1: i64 lower_boundary;
                    999: i64 upper_boundary;
                    1000: i64 overflow;
                }

                @internal.InjectMetadataFields{type="BoundaryFields"}
                struct Injected6 {}
                """
            ),
        )

        ret, out, err = self.run_thrift("bar.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[WARNING:bar.thrift:36] Nonpositive value (-1) not allowed as a field id.\n"
            "[FAILURE:bar.thrift:17] Can not find expected type `foo.Fields` specified"
            " in `@internal.InjectMetadataFields` in the current scope. Please check the include.\n"
            "[FAILURE:bar.thrift:20] key `type` not found.\n"
            "[FAILURE:bar.thrift:23] `bar.UnionFields` is not a struct type. "
            "`@internal.InjectMetadataFields` can be only used with a struct type.\n"
            "[FAILURE:bar.thrift:26] `bar.MyI64` is not a struct type. "
            "`@internal.InjectMetadataFields` can be only used with a struct type.\n"
            "[FAILURE:bar.thrift:43] Field id `-1` does not mapped to valid internal id.\n"
            "[FAILURE:bar.thrift:43] Field id `1000` does not mapped to valid internal id.\n"
            "[FAILURE:bar.thrift] Field `field1` is already defined for `Injected5`.\n",
        )

    def test_set_invalid_elem_type(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct S {
                    1: i32 field,
                    2: set<float> set_of_float,
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift", gen="hack")
        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift] InvalidKeyType: Hack only supports integers and strings as key for map and set - https://fburl.com/wiki/pgzirbu8, field: set<float> set_of_float.\n",
        )

    def test_map_invalid_key_type(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct S {
                    1: i32 field,
                    2: map<float, i32> map_of_float_to_int,
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift", gen="hack")
        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift] InvalidKeyType: Hack only supports integers and strings as key for map and set - https://fburl.com/wiki/pgzirbu8, field: map<float, i32> map_of_float_to_int.\n",
        )

    def test_rpc_return_invalid_key_type(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                service Foo {
                    set<float> invalid_rpc_return();
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift", gen="hack")
        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift] InvalidKeyType: Hack only supports integers and strings as key for map and set - https://fburl.com/wiki/pgzirbu8, function invalid_rpc_return has invalid return type with type: set<float>.\n",
        )

    def test_rpc_param_invalid_key_type(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                service Foo {
                    void invalid_rpc_param(set<float> arg1);
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift", gen="hack")
        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift] InvalidKeyType: Hack only supports integers and strings as key for map and set - https://fburl.com/wiki/pgzirbu8, function invalid_rpc_param has invalid param arg1 with type: set<float>.\n",
        )
