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
