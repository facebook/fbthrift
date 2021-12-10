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

from __future__ import absolute_import, division, print_function, unicode_literals

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
thrift = ascend_find_exe(exe, "thrift")


def read_file(path):
    with open(path, "r") as f:
        return f.read()


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

    def test_neg_enum_value(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                enum Foo {
                    Bar = -1;
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(
            err,
            "[WARNING:foo.thrift:2] Negative value supplied for enum value `Bar`.\n",
        )
        self.assertEqual(ret, 0)

        ret, out, err = self.run_thrift("--allow-neg-enum-vals", "foo.thrift")
        self.assertEqual(err, "")
        self.assertEqual(ret, 0)

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
            "[WARNING:foo.thrift:2] Nonpositive field id (0) differs from what is auto-assigned by thrift. The id must positive or -1.\n"
            * 2
            + "[WARNING:foo.thrift:2] No field id specified for `field`, resulting protocol may have conflicts or not be backwards compatible!\n",
        )
        self.assertEqual(ret, 0)

        ret, out, err = self.run_thrift("--allow-neg-keys", "foo.thrift")
        self.assertEqual(
            err,
            "[WARNING:foo.thrift:2] Nonpositive field id (0) differs from what would be auto-assigned by thrift (-1).\n"
            * 2
            + "[FAILURE:foo.thrift:2] Zero value (0) not allowed as a field id for `field`\n",
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
            "[WARNING:foo.thrift:2] Nonpositive field id (0) differs from what is auto-assigned by thrift. The id must positive or -1.\n"
            * 2
            + "[WARNING:foo.thrift:2] No field id specified for `field`, resulting protocol may have conflicts or not be backwards compatible!\n",
        )
        self.assertEqual(ret, 0)

        ret, out, err = self.run_thrift("--allow-neg-keys", "foo.thrift")
        self.assertEqual(
            err,
            "[WARNING:foo.thrift:2] Nonpositive field id (0) differs from what would be auto-assigned by thrift (-1).\n"
            * 2,
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
                    -16384: i32 f3; // min value.
                    -16385: i32 f4; // min value - 1.
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(
            err,
            "[WARNING:foo.thrift:3] Nonpositive value (-2) not allowed as a field id.\n"
            "[WARNING:foo.thrift:4] Nonpositive field id (-16384) differs from what is auto-assigned by thrift. The id must positive or -3.\n"
            "[WARNING:foo.thrift:5] Nonpositive field id (-16385) differs from what is auto-assigned by thrift. The id must positive or -4.\n"
            * 2
            + "[WARNING:foo.thrift:2] No field id specified for `f1`, resulting protocol may have conflicts or not be backwards compatible!\n"
            "[WARNING:foo.thrift:4] No field id specified for `f3`, resulting protocol may have conflicts or not be backwards compatible!\n"
            "[WARNING:foo.thrift:5] No field id specified for `f4`, resulting protocol may have conflicts or not be backwards compatible!\n",
        )
        self.assertEqual(ret, 0)

        ret, out, err = self.run_thrift("--allow-neg-keys", "foo.thrift")
        self.assertEqual(
            err,
            "[WARNING:foo.thrift:4] Nonpositive field id (-16384) differs from what would be auto-assigned by thrift (-3).\n"
            * 2
            + "[WARNING:foo.thrift:2] No field id specified for `f1`, resulting protocol may have conflicts or not be backwards compatible!\n"
            "[FAILURE:foo.thrift:5] Reserved field id (-16385) cannot be used for `f4`.\n",
        )
        self.assertEqual(ret, 1)

    def test_exhausted_field_ids(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct Foo {
                    -16384: i32 f1; // min value.
                    i32 f2; // auto id = -2 or min value - 1.
                }
                """
            ),
        )
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(
            err,
            "[WARNING:foo.thrift:2] Nonpositive field id (-16384) differs from what is auto-assigned by thrift. The id must positive or -1.\n"
            * 2
            + "[WARNING:foo.thrift:2] No field id specified for `f1`, resulting protocol may have conflicts or not be backwards compatible!\n"
            "[WARNING:foo.thrift:3] No field id specified for `f2`, resulting protocol may have conflicts or not be backwards compatible!\n",
        )
        self.assertEqual(ret, 0)

        ret, out, err = self.run_thrift("--allow-neg-keys", "foo.thrift")
        self.assertEqual(
            err,
            "[WARNING:foo.thrift:2] Nonpositive field id (-16384) differs from what would be auto-assigned by thrift (-1).\n"
            "[FAILURE:foo.thrift:3] Cannot allocate an id for `f2`. Automatic field ids are exhausted.\n",
        )
        self.assertEqual(ret, 1)

    def test_too_many_fields(self):
        reserved_id = int.from_bytes(
            [int("10111111", 2), 255], byteorder="big", signed=True
        )
        id_count = -reserved_id
        lines = ["struct Foo {"] + [f"i32 field_{i}" for i in range(id_count)] + ["}"]
        write_file("foo.thrift", "\n".join(lines))

        expected_error = [
            f"[FAILURE:foo.thrift:{id_count + 2}] Cannot allocate an id for `field_{id_count - 1}`. Automatic field ids are exhausted."
        ]
        expected_error = "\n".join(expected_error) + "\n"

        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(ret, 1)
        self.assertEqual(err, expected_error)

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
            "[WARNING:overflow.thrift:2] Nonpositive field id (-32768) differs from what is auto-assigned by thrift. The id must positive or -1.\n"
            "[FAILURE:overflow.thrift:4] Integer constant (32768) outside the range of field ids ([-32768, 32767]).\n",
        )
        self.assertEqual(ret, 1)
        ret, out, err = self.run_thrift("underflow.thrift")
        self.assertEqual(
            err,
            "[WARNING:underflow.thrift:2] Nonpositive field id (-32768) differs from what is auto-assigned by thrift. The id must positive or -1.\n"
            "[FAILURE:underflow.thrift:4] Integer constant (-32769) outside the range of field ids ([-32768, 32767]).\n",
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
            "terminate called after throwing an instance of 'std::runtime_error'\n"
            "  what():  Oneway methods must have void return type: bar\n",
        )
        self.assertEqual(ret, -6)

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
            "terminate called after throwing an instance of 'std::runtime_error'\n"
            "  what():  Oneway methods can't throw exceptions: baz\n",
        )
        self.assertEqual(ret, -6)

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
            "[FAILURE:foo.thrift:9] type error: const `badValMap<val>` was declared as i32.\n"
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
            "[FAILURE:foo.thrift:8] type error: const `badInt` was declared as i32.\n"
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
            err, "[FAILURE:foo.thrift:4] Type `Random.Type` not defined.\n"
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
            "[WARNING:foo.thrift:2] cpp.ref, cpp2.ref are deprecated. Please use thrift.box annotation instead in `rec`.\n"
        )

    def test_structured_ref(self):
        write_file(
            "thrift/annotation/cpp.thrift",
            textwrap.dedent(
                """\
                enum RefType {Unique, SharedConst, SharedMutable}
                struct Ref {
                    1: RefType type;
                } (thrift.uri = "facebook.com/thrift/annotation/cpp/Ref")
                """
            ),
        )

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
            '\n' + err,
            textwrap.dedent("""
                [WARNING:foo.thrift:4] cpp.ref, cpp2.ref are deprecated. Please use thrift.box annotation instead in `field1`.
                [WARNING:foo.thrift:7] @cpp.Ref{type = cpp.RefType.Unique} is deprecated. Please use thrift.box annotation instead in `field2`.
                [FAILURE:foo.thrift:10] The @cpp.Ref annotation cannot be combined with the `cpp.ref` or `cpp.ref_type` annotations. Remove one of the annotations from `field3`.
                [WARNING:foo.thrift:10] cpp.ref, cpp2.ref are deprecated. Please use thrift.box annotation instead in `field3`.
                [WARNING:foo.thrift:10] @cpp.Ref{type = cpp.RefType.Unique} is deprecated. Please use thrift.box annotation instead in `field3`.
                [WARNING:foo.thrift:14] @cpp.Ref{type = cpp.RefType.Unique} is deprecated. Please use thrift.box annotation instead in `field4`.
                [FAILURE:foo.thrift:13] Structured annotation `Ref` is already defined for `field4`.
            """)
        )

    def test_adapter(self):
        write_file(
            "thrift/annotation/cpp.thrift",
            textwrap.dedent(
                """\
                struct Adapter {
                    1: string name;
                } (thrift.uri = "facebook.com/thrift/annotation/cpp/Adapter")
                """
            ),
        )

        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                include "thrift/annotation/cpp.thrift"

                typedef i64 MyI64 (cpp.adapter="MyAdapter")

                struct MyStruct {
                    @cpp.Adapter{name="MyAdapter"}
                    1: MyI64 my_field;
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:7] `@cpp.Adapter` cannot be combined with "
            "`cpp_adapter` in `my_field`.\n",
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
                [WARNING:foo.thrift:3] cpp.ref, cpp2.ref are deprecated. Please use thrift.box annotation instead in `a`.
                """
            ),
        )

    def test_bitpack_with_tablebased_seriliazation(self):
        write_file(
            "thrift/annotation/cpp.thrift",
            textwrap.dedent(
                """\
                struct PackIsset {
                } (thrift.uri = "facebook.com/thrift/annotation/cpp/PackIsset")
                """
            ),
        )

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

        ret, out, err = self.run_thrift("foo.thrift",gen="mstch_cpp2:json,tablebased")

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

    def test_unordered_minimize_padding(self):
        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                struct A {
                    1: B field
                } (cpp.minimize_padding)
                struct B {
                1: i32 x
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            textwrap.dedent(
                "[FAILURE:foo.thrift] cpp.minimize_padding requires struct "
                "definitions to be topologically sorted. Move definition of "
                "`B` before its use in field `field`.\n"
            ),
        )

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
                    1: optional i64 field (cpp.ref, cpp.box)
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            textwrap.dedent(
                "[WARNING:foo.thrift:1] Cpp.box is deprecated. Please use thrift.box annotation instead in `field`.\n"
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
                    1: i64 field (cpp.box)
                }
                """
            ),
        )

        ret, out, err = self.run_thrift("foo.thrift")

        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            textwrap.dedent(
                "[WARNING:foo.thrift:1] Cpp.box is deprecated. Please use thrift.box annotation instead in `field`.\n"
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
            "scope.thrift",
            textwrap.dedent(
                """
                struct Struct {
                } (thrift.uri = "facebook.com/thrift/annotation/Struct")
                struct Union {
                } (thrift.uri = "facebook.com/thrift/annotation/Union")
                struct Exception {
                } (thrift.uri = "facebook.com/thrift/annotation/Exception")
                struct Field {
                } (thrift.uri = "facebook.com/thrift/annotation/Field")
                struct Typedef {
                } (thrift.uri = "facebook.com/thrift/annotation/Typedef")
                struct Service {
                } (thrift.uri = "facebook.com/thrift/annotation/Service")
                struct Interaction {
                } (thrift.uri = "facebook.com/thrift/annotation/Interaction")
                struct Function {
                } (thrift.uri = "facebook.com/thrift/annotation/Function")
                struct EnumValue {
                } (thrift.uri = "facebook.com/thrift/annotation/EnumValue")
                struct Const {
                } (thrift.uri = "facebook.com/thrift/annotation/Const")

                // Due to cython bug, we can not use `Enum` as class name directly
                // https://github.com/cython/cython/issues/2474
                struct FbthriftInternalEnum {}
                typedef FbthriftInternalEnum Enum (thrift.uri = "facebook.com/thrift/annotation/Enum")
                """
            ),
        )

        write_file(
            "foo.thrift",
            textwrap.dedent(
                """\
                include "scope.thrift"

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
        self.assertEqual(
            "\n" + err,
            textwrap.dedent(
                """
                [WARNING:foo.thrift:18] Using `NotAnAnnot` as an annotation, even though it has not been enabled for any annotation scope.
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
                [FAILURE:file2.thrift:2] thrift.uri `facebook.com/thrift/annotation/Foo` is already defined for `Foo2`.
                [FAILURE:main.thrift:3] thrift.uri `facebook.com/thrift/annotation/Bar` is already defined for `Bar2`.
                [FAILURE:main.thrift:7] thrift.uri `facebook.com/thrift/annotation/Baz` is already defined for `Baz2`.
                """
            ),
        )

    def test_unique_ref(self):
        write_file(
            "thrift/annotation/cpp.thrift",
            textwrap.dedent(
                """\
                enum RefType {Unique, SharedConst, SharedMutable}
                struct Ref {
                    1: RefType type;
                } (thrift.uri = "facebook.com/thrift/annotation/cpp/Ref")
                """
            ),
        )

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

                    @cpp.Ref{type = cpp.RefType.SharedConst}
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
                [WARNING:foo.thrift:4] cpp.ref, cpp2.ref are deprecated. Please use thrift.box annotation instead in `field1`.
                [WARNING:foo.thrift:6] cpp.ref, cpp2.ref are deprecated. Please use thrift.box annotation instead in `field2`.
                [WARNING:foo.thrift:9] @cpp.Ref{type = cpp.RefType.Unique} is deprecated. Please use thrift.box annotation instead in `field3`.
                [WARNING:foo.thrift:17] cpp.ref_type = `unique`, cpp2.ref_type = `unique` are deprecated. Please use thrift.box annotation instead in `field6`.
                [WARNING:foo.thrift:19] cpp.ref_type = `unique`, cpp2.ref_type = `unique` are deprecated. Please use thrift.box annotation instead in `field7`.
                [WARNING:foo.thrift:25] cpp.ref, cpp2.ref are deprecated. Please use thrift.box annotation instead in `field10`.
                [WARNING:foo.thrift:27] cpp.ref, cpp2.ref are deprecated. Please use thrift.box annotation instead in `field11`.
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
                [WARNING:foo.thrift:1] Cpp.box is deprecated. Please use thrift.box annotation instead in `field`.
                """
            ),
        )
