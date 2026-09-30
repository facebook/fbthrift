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

# pyre-unsafe

import os
import shutil
import tempfile
import textwrap
import unittest

import pkg_resources
from xplat.thrift.compiler.codemod.test_utils import read_file, run_binary, write_file


class MigrateCppContainerTypesTest(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, tmp, True)
        self.addCleanup(os.chdir, os.getcwd())
        os.chdir(tmp)
        self.maxDiff = None

        write_file(
            "thrift/annotation/cpp.thrift",
            textwrap.dedent(
                """\
                package "facebook.com/thrift/annotation/cpp"

                struct Type {
                    1: string name;
                    2: string template;
                }

                struct Adapter {
                    1: string name;
                }
                """
            ),
        )

    def test_migrates_only_equivalent_container_types(self):
        before = textwrap.dedent(
            """\
            include "thrift/annotation/cpp.thrift"

            namespace cpp2 example

            struct Item {}

            @cpp.Type{name = "std::vector<int32_t>"}
            typedef list<i32> Ints

            @cpp.Type{name = "folly::F14FastMap<std::string, double>"}
            typedef map<string, double> Scores

            @cpp.Type{name = "std::vector<std::vector<int32_t>>"}
            typedef list<list<i32>> Rows

            @cpp.Type{name = "std::vector<std::vector<uint32_t>>"}
            typedef list<list<i32>> UnsignedRows

            @cpp.Type{name = "std::vector<mystd::int32_t>"}
            typedef list<i32> IdentifierBoundary

            @cpp.Type{name = "std::vector<vector<int32_t>>"}
            typedef list<list<i32>> UnqualifiedCustomInnerContainer

            @cpp.Type{
              name = "std::vector<int32_t>",
              template = "std::vector",
            }
            typedef list<i32> AlreadyHasTemplate

            struct Record {
              @cpp.Type{name = "std::unordered_set<std::string>"}
              1: set<string> tags;

              @cpp.Type{name = "std::vector<uint32_t>"}
              2: list<i32> unsigned_values;

              @cpp.Type{name = "folly::small_vector<int32_t, 7>"}
              3: list<i32> inline_values;

              @cpp.Type{
                name = "std::map<std::string, double, CustomComparator>"
              }
              4: map<string, double> sorted_values;

              @cpp.Type{name = "std::vector<Item>"}
              5: list<Item> items;

            }
            """
        )
        expected = before.replace(
            '@cpp.Type{name = "std::vector<int32_t>"}',
            '@cpp.Type{template = "std::vector"}',
        ).replace(
            '@cpp.Type{name = "folly::F14FastMap<std::string, double>"}',
            '@cpp.Type{template = "folly::F14FastMap"}',
        ).replace(
            '@cpp.Type{name = "std::unordered_set<std::string>"}',
            '@cpp.Type{template = "std::unordered_set"}',
        ).replace(
            '@cpp.Type{name = "std::vector<std::vector<int32_t>>"}',
            '@cpp.Type{template = "std::vector"}',
        ).replace(
            '@cpp.Type{name = "std::vector<std::vector<uint32_t>>"}',
            '@cpp.Type{template = "std::vector"}',
        ).replace(
            "typedef list<list<i32>> UnsignedRows",
            "typedef list<list<uint32>> UnsignedRows",
        ).replace(
            "2: list<i32> unsigned_values;",
            "2: list<uint32> unsigned_values;",
        ).replace(
            '@cpp.Type{name = "std::vector<uint32_t>"}',
            '@cpp.Type{template = "std::vector"}',
        ).replace(
            '@cpp.Type{name = "std::vector<Item>"}',
            '@cpp.Type{template = "std::vector"}',
        ) + textwrap.dedent(
            """\

                @cpp.Type{name = "uint32_t"}
                typedef i32 uint32
                """
        )

        write_file("foo.thrift", before)
        binary = pkg_resources.resource_filename(__name__, "codemod")
        run_binary(binary, "foo.thrift")

        self.assertEqual(read_file("foo.thrift"), expected)

        run_binary(binary, "foo.thrift")
        self.assertEqual(read_file("foo.thrift"), expected)

    def test_extracts_typedefs_for_primitive_inner_types(self):
        before = textwrap.dedent(
            """\
            include "thrift/annotation/cpp.thrift"

            namespace cpp2 example

            @cpp.Type{name = "std::vector<uint32_t>"}
            typedef list<i32> UnsignedInts

            @cpp.Type{name = "uint32_t"}
            typedef i32 uint32

            @cpp.Adapter{name = "Adapter"}
            @cpp.Type{name = "uint16_t"}
            typedef i16 adapted_uint16

            typedef list<i32> IntList

            struct Record {
              @cpp.Type{name = "std::vector<std::uint32_t>"}
              1: list<i32> unsigned_values;

              @cpp.Type{name = "std::map<uint16_t, std::vector<uint64_t>>"}
              2: map<i16, list<i64>> nested_values;

              @cpp.Type{name = "std::vector<uint64_t>"}
              3: list<i32> wrong_width;

              @cpp.Type{name = "folly::small_vector<uint32_t, 7>"}
              4: list<i32> extra_template_argument;

              @cpp.Type{name = "std::vector<uint32_t>"}
              5: IntList aliased_container;
            }
            """
        )
        expected = textwrap.dedent(
            """\
            include "thrift/annotation/cpp.thrift"

            namespace cpp2 example

            @cpp.Type{template = "std::vector"}
            typedef list<uint32> UnsignedInts

            @cpp.Type{name = "uint32_t"}
            typedef i32 uint32

            @cpp.Adapter{name = "Adapter"}
            @cpp.Type{name = "uint16_t"}
            typedef i16 adapted_uint16

            typedef list<i32> IntList

            struct Record {
              @cpp.Type{template = "std::vector"}
              1: list<uint32> unsigned_values;

              @cpp.Type{template = "std::map"}
              2: map<uint16, list<uint64>> nested_values;

              @cpp.Type{name = "std::vector<uint64_t>"}
              3: list<i32> wrong_width;

              @cpp.Type{name = "folly::small_vector<uint32_t, 7>"}
              4: list<i32> extra_template_argument;

              @cpp.Type{name = "std::vector<uint32_t>"}
              5: IntList aliased_container;
            }

            @cpp.Type{name = "uint16_t"}
            typedef i16 uint16

            @cpp.Type{name = "uint64_t"}
            typedef i64 uint64
            """
        )

        write_file("foo.thrift", before)
        binary = pkg_resources.resource_filename(__name__, "codemod")
        run_binary(binary, "foo.thrift")
        self.assertEqual(read_file("foo.thrift"), expected)

        run_binary(binary, "foo.thrift")
        self.assertEqual(read_file("foo.thrift"), expected)

    def test_treats_equivalent_integer_spellings_as_identical(self):
        before = textwrap.dedent(
            """\
            include "thrift/annotation/cpp.thrift"

            namespace cpp2 example

            struct Record {
              @cpp.Type{name = "std::unordered_map<std::string, int>"}
              1: map<string, i32> plain_int;

              @cpp.Type{name = "std::vector<::std::int32_t>"}
              2: list<i32> global_std_int;

              @cpp.Type{name = "std::vector< signed  int >"}
              3: list<i32> signed_int;

              @cpp.Type{name = "std::vector<short>"}
              4: list<i16> shorts;

              @cpp.Type{name = "std::vector<signed char>"}
              5: list<byte> signed_chars;

              @cpp.Type{name = "::std::vector<::int64_t>"}
              6: list<i64> global_int64;

              @cpp.Type{name = "std::vector<unsigned>"}
              7: list<i32> unsigned_ints;

              @cpp.Type{name = "std::vector<unsigned short int>"}
              8: list<i16> unsigned_shorts;

              @cpp.Type{name = "std::vector<long>"}
              9: list<i64> longs;

              @cpp.Type{name = "std::vector<long long>"}
              10: list<i64> long_longs;

              @cpp.Type{name = "std::vector<unsigned long long>"}
              11: list<i64> unsigned_long_longs;

              @cpp.Type{name = "std::vector<char>"}
              12: list<byte> chars;

              @cpp.Type{name = "std::vector<int>"}
              13: list<i64> int_for_i64;

              @cpp.Type{name = "std::vector<const int>"}
              14: list<i32> const_ints;

              @cpp.Type{name = "std::vector<int*>"}
              15: list<i32> int_pointers;
            }
            """
        )
        expected = textwrap.dedent(
            """\
            include "thrift/annotation/cpp.thrift"

            namespace cpp2 example

            struct Record {
              @cpp.Type{template = "std::unordered_map"}
              1: map<string, i32> plain_int;

              @cpp.Type{template = "std::vector"}
              2: list<i32> global_std_int;

              @cpp.Type{template = "std::vector"}
              3: list<i32> signed_int;

              @cpp.Type{template = "std::vector"}
              4: list<i16> shorts;

              @cpp.Type{template = "std::vector"}
              5: list<byte> signed_chars;

              @cpp.Type{template = "::std::vector"}
              6: list<i64> global_int64;

              @cpp.Type{template = "std::vector"}
              7: list<uint32> unsigned_ints;

              @cpp.Type{template = "std::vector"}
              8: list<uint16> unsigned_shorts;

              @cpp.Type{name = "std::vector<long>"}
              9: list<i64> longs;

              @cpp.Type{name = "std::vector<long long>"}
              10: list<i64> long_longs;

              @cpp.Type{name = "std::vector<unsigned long long>"}
              11: list<i64> unsigned_long_longs;

              @cpp.Type{name = "std::vector<char>"}
              12: list<byte> chars;

              @cpp.Type{name = "std::vector<int>"}
              13: list<i64> int_for_i64;

              @cpp.Type{name = "std::vector<const int>"}
              14: list<i32> const_ints;

              @cpp.Type{name = "std::vector<int*>"}
              15: list<i32> int_pointers;
            }

            @cpp.Type{name = "uint32_t"}
            typedef i32 uint32

            @cpp.Type{name = "uint16_t"}
            typedef i16 uint16
            """
        )

        write_file("foo.thrift", before)
        binary = pkg_resources.resource_filename(__name__, "codemod")
        run_binary(binary, "foo.thrift")
        self.assertEqual(read_file("foo.thrift"), expected)

        run_binary(binary, "foo.thrift")
        self.assertEqual(read_file("foo.thrift"), expected)

    def test_resolves_named_element_types(self):
        write_file(
            "inc.thrift",
            textwrap.dedent(
                """\
                include "thrift/annotation/cpp.thrift"

                namespace cpp2 inc.ns

                enum Color {
                  RED = 0,
                }

                struct Thing {}

                typedef string Name

                @cpp.Type{name = "folly::fbstring"}
                typedef string FbName
                """
            ),
        )
        before = textwrap.dedent(
            """\
            include "thrift/annotation/cpp.thrift"
            include "inc.thrift"

            namespace cpp2 example

            enum ServiceToLimit {
              NOVAK2 = 0,
            }

            struct Item {}

            union Choice {
              1: i32 value;
            }

            typedef string LocalName

            typedef LocalName LocalNameAlias

            struct Record {
              @cpp.Type{name = "std::unordered_map<ServiceToLimit, int>"}
              1: map<ServiceToLimit, i32> enum_key;

              @cpp.Type{name = "std::vector<::example::Item>"}
              2: list<Item> qualified_struct;

              @cpp.Type{name = "std::vector<example::Choice>"}
              3: list<Choice> qualified_union;

              @cpp.Type{name = "folly::F14FastSet<std::string>"}
              4: set<LocalNameAlias> typedef_chain;

              @cpp.Type{name = "std::vector<inc::ns::Color>"}
              5: list<inc.Color> included_enum;

              @cpp.Type{name = "std::unordered_set<std::string>"}
              6: set<inc.Name> included_typedef;

              @cpp.Type{name = "std::vector<folly::fbstring>"}
              7: list<inc.FbName> annotated_typedef;

              @cpp.Type{name = "std::vector<LocalName>"}
              8: list<string> typedef_in_annotation;

              @cpp.Type{name = "std::vector<Color>"}
              9: list<inc.Color> unqualified_included_enum;

              @cpp.Type{name = "std::vector<Item>"}
              10: list<Choice> mismatched_element;

              @cpp.Type{name = "std::vector<std::string>"}
              11: list<inc.FbName> mismatched_annotated_typedef;

              @cpp.Type{name = "std::vector<other::Item>"}
              12: list<Item> wrong_namespace;

              @cpp.Type{name = "std::vector<ns::Thing>"}
              13: list<inc.Thing> partially_qualified_outside_scope;
            }
            """
        )
        expected = textwrap.dedent(
            """\
            include "thrift/annotation/cpp.thrift"
            include "inc.thrift"

            namespace cpp2 example

            enum ServiceToLimit {
              NOVAK2 = 0,
            }

            struct Item {}

            union Choice {
              1: i32 value;
            }

            typedef string LocalName

            typedef LocalName LocalNameAlias

            struct Record {
              @cpp.Type{template = "std::unordered_map"}
              1: map<ServiceToLimit, i32> enum_key;

              @cpp.Type{template = "std::vector"}
              2: list<Item> qualified_struct;

              @cpp.Type{template = "std::vector"}
              3: list<Choice> qualified_union;

              @cpp.Type{template = "folly::F14FastSet"}
              4: set<LocalNameAlias> typedef_chain;

              @cpp.Type{template = "std::vector"}
              5: list<inc.Color> included_enum;

              @cpp.Type{template = "std::unordered_set"}
              6: set<inc.Name> included_typedef;

              @cpp.Type{template = "std::vector"}
              7: list<inc.FbName> annotated_typedef;

              @cpp.Type{template = "std::vector"}
              8: list<string> typedef_in_annotation;

              @cpp.Type{name = "std::vector<Color>"}
              9: list<inc.Color> unqualified_included_enum;

              @cpp.Type{name = "std::vector<Item>"}
              10: list<Choice> mismatched_element;

              @cpp.Type{name = "std::vector<std::string>"}
              11: list<inc.FbName> mismatched_annotated_typedef;

              @cpp.Type{name = "std::vector<other::Item>"}
              12: list<Item> wrong_namespace;

              @cpp.Type{name = "std::vector<ns::Thing>"}
              13: list<inc.Thing> partially_qualified_outside_scope;
            }
            """
        )

        write_file("foo.thrift", before)
        binary = pkg_resources.resource_filename(__name__, "codemod")
        run_binary(binary, "foo.thrift")
        self.assertEqual(read_file("foo.thrift"), expected)

        run_binary(binary, "foo.thrift")
        self.assertEqual(read_file("foo.thrift"), expected)

    def test_resolves_names_relative_to_enclosing_namespaces(self):
        write_file(
            "inc.thrift",
            textwrap.dedent(
                """\
                namespace cpp2 inc.ns

                enum Color {
                  RED = 0,
                }

                struct Thing {}
                """
            ),
        )
        before = textwrap.dedent(
            """\
            include "thrift/annotation/cpp.thrift"
            include "inc.thrift"

            namespace cpp2 inc.app

            struct Color {}

            struct Record {
              @cpp.Type{name = "std::vector<ns::Thing>"}
              1: list<inc.Thing> partially_qualified;

              @cpp.Type{name = "std::vector<Color>"}
              2: list<inc.Color> shadowed;

              @cpp.Type{name = "std::vector<Color>"}
              3: list<Color> local;
            }
            """
        )
        expected = before.replace(
            '@cpp.Type{name = "std::vector<ns::Thing>"}',
            '@cpp.Type{template = "std::vector"}',
        ).replace(
            '@cpp.Type{name = "std::vector<Color>"}\n  3:',
            '@cpp.Type{template = "std::vector"}\n  3:',
        )

        write_file("foo.thrift", before)
        binary = pkg_resources.resource_filename(__name__, "codemod")
        run_binary(binary, "foo.thrift")
        self.assertEqual(read_file("foo.thrift"), expected)

    def test_migrates_containers_of_annotated_typedefs(self):
        before = textwrap.dedent(
            """\
            include "thrift/annotation/cpp.thrift"

            namespace cpp2 example

            @cpp.Type{name = "std::unordered_set<std::string>"}
            typedef set<string> UnorderedSetString

            @cpp.Type{
              name = "std::unordered_map<std::string,std::unordered_set<std::string>>",
            }
            typedef map<string, UnorderedSetString> UnorderedMapStringUnorderedSetString

            @cpp.Type{template = "std::unordered_set"}
            typedef set<i32> TemplatedSet

            @cpp.Type{name = "std::vector<std::unordered_set<int>>"}
            typedef list<TemplatedSet> TemplatedSets

            @cpp.Type{template = "my::Set"}
            typedef set<i32> CustomSet

            @cpp.Type{name = "std::vector<std::unordered_set<int>>"}
            typedef list<CustomSet> CustomSets

            typedef set<string> PlainSet

            @cpp.Type{name = "std::vector<std::unordered_set<std::string>>"}
            typedef list<PlainSet> PlainSets
            """
        )
        expected = (
            before.replace(
                '@cpp.Type{name = "std::unordered_set<std::string>"}',
                '@cpp.Type{template = "std::unordered_set"}',
            )
            .replace(
                'name = "std::unordered_map<std::string,std::unordered_set<std::string>>",',
                'template = "std::unordered_map",',
            )
            .replace(
                '@cpp.Type{name = "std::vector<std::unordered_set<int>>"}\n'
                "typedef list<TemplatedSet>",
                '@cpp.Type{template = "std::vector"}\ntypedef list<TemplatedSet>',
            )
        )

        write_file("foo.thrift", before)
        binary = pkg_resources.resource_filename(__name__, "codemod")
        run_binary(binary, "foo.thrift")
        self.assertEqual(read_file("foo.thrift"), expected)

        run_binary(binary, "foo.thrift")
        self.assertEqual(read_file("foo.thrift"), expected)

    def test_extracts_typedefs_for_inline_nested_containers(self):
        before = textwrap.dedent(
            """\
            include "thrift/annotation/cpp.thrift"

            namespace cpp2 example

            @cpp.Type{name = "std::unordered_set<std::string>"}
            typedef set<string> ExistingSet

            struct folly_F14FastSet_uint64 {}

            struct Record {
              @cpp.Type{
                name = "std::unordered_map<std::string, std::unordered_set<std::string>>"
              }
              1: map<string, set<string>> reused;

              @cpp.Type{name = "std::vector<std::unordered_set<int64_t>>"}
              2: list<set<i64>> extracted;

              @cpp.Type{name = "std::map<uint16_t, folly::F14FastSet<uint64_t>>"}
              3: map<i16, set<i64>> nested_primitive;

              @cpp.Type{name = "std::vector<std::vector<std::unordered_set<long>>>"}
              4: list<list<set<i64>>> deeper_mismatch;

              @cpp.Type{
                name = "std::vector<std::vector<std::unordered_set<int64_t>>>"
              }
              5: list<list<set<i64>>> deeper;

              @cpp.Type{name = "std::vector<std::unordered_set<int64_t>>"}
              6: list<set<i32>> mismatched_inner;

              @cpp.Type{name = "std::vector<folly::small_vector<int64_t, 2>>"}
              7: list<list<i64>> extra_argument;

              @cpp.Type{name = "std::vector<unordered_set<int64_t>>"}
              8: list<set<i64>> unqualified_template;
            }
            """
        )
        expected = textwrap.dedent(
            """\
            include "thrift/annotation/cpp.thrift"

            namespace cpp2 example

            @cpp.Type{template = "std::unordered_set"}
            typedef set<string> ExistingSet

            struct folly_F14FastSet_uint64 {}

            struct Record {
              @cpp.Type{
                template = "std::unordered_map"
              }
              1: map<string, ExistingSet> reused;

              @cpp.Type{template = "std::vector"}
              2: list<std_unordered_set_int64> extracted;

              @cpp.Type{template = "std::map"}
              3: map<uint16, folly_F14FastSet_uint64_2> nested_primitive;

              @cpp.Type{name = "std::vector<std::vector<std::unordered_set<long>>>"}
              4: list<list<set<i64>>> deeper_mismatch;

              @cpp.Type{
                template = "std::vector"
              }
              5: list<list<std_unordered_set_int64>> deeper;

              @cpp.Type{name = "std::vector<std::unordered_set<int64_t>>"}
              6: list<set<i32>> mismatched_inner;

              @cpp.Type{name = "std::vector<folly::small_vector<int64_t, 2>>"}
              7: list<list<i64>> extra_argument;

              @cpp.Type{name = "std::vector<unordered_set<int64_t>>"}
              8: list<set<i64>> unqualified_template;
            }

            @cpp.Type{template = "std::unordered_set"}
            typedef set<i64> std_unordered_set_int64

            @cpp.Type{name = "uint16_t"}
            typedef i16 uint16

            @cpp.Type{name = "uint64_t"}
            typedef i64 uint64

            @cpp.Type{template = "folly::F14FastSet"}
            typedef set<uint64> folly_F14FastSet_uint64_2
            """
        )

        write_file("foo.thrift", before)
        binary = pkg_resources.resource_filename(__name__, "codemod")
        run_binary(binary, "foo.thrift")
        self.assertEqual(read_file("foo.thrift"), expected)

        run_binary(binary, "foo.thrift")
        self.assertEqual(read_file("foo.thrift"), expected)

    def test_resolves_adapted_struct_fields_in_extra_namespace(self):
        before = textwrap.dedent(
            """\
            include "thrift/annotation/cpp.thrift"

            namespace cpp2 example

            struct Item {}

            @cpp.Adapter{name = "::my::Adapter"}
            struct AdaptedItem {}

            @cpp.Adapter{name = "::my::Adapter"}
            struct Adapted {
              @cpp.Type{name = "std::vector<Item>"}
              1: list<Item> items;

              @cpp.Type{name = "std::vector<AdaptedItem>"}
              2: list<AdaptedItem> shadowed_by_underlying_class;

              @cpp.Type{name = "std::vector<::example::AdaptedItem>"}
              3: list<AdaptedItem> qualified_adapted_items;
            }
            """
        )
        expected = before.replace(
            '@cpp.Type{name = "std::vector<Item>"}',
            '@cpp.Type{template = "std::vector"}',
        ).replace(
            '@cpp.Type{name = "std::vector<::example::AdaptedItem>"}',
            '@cpp.Type{template = "std::vector"}',
        )

        write_file("foo.thrift", before)
        binary = pkg_resources.resource_filename(__name__, "codemod")
        run_binary(binary, "foo.thrift")
        self.assertEqual(read_file("foo.thrift"), expected)
