/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

namespace cpp2 apache.thrift.test

cpp_include "thrift/test/CppAllocatorTest.h"

struct aa_struct2 {
  1: list<i32> (cpp.use_allocator, cpp.template = "MyVector") aa_list;
  2: set<i32> (cpp.use_allocator, cpp.template = "MySet") aa_set;
  3: map<i32, i32> (cpp.use_allocator, cpp.template = "MyMap") aa_map;
  4: string (cpp_use_allocator, cpp.type = "MyString") aa_string;
  5: i32 not_a_container;
  6: list<i32> not_aa_list;
  7: set<i32> not_aa_set;
  8: map<i32, i32> not_aa_map;
  9: string not_aa_string;
} (cpp.allocator="MyAlloc")

struct aa_struct {
  1: aa_struct2 (cpp.use_allocator) nested;
} (cpp.allocator="MyAlloc")
