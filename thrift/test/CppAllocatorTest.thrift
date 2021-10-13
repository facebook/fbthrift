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

struct AlwaysThrowChild {
  1: list<i32> (
    cpp.use_allocator,
    cpp.template = "::AlwaysThrowVector",
  ) aa_list;
  2: set<i32> (cpp.use_allocator, cpp.template = "::AlwaysThrowSet") aa_set;
  3: map<i32, i32> (
    cpp.use_allocator,
    cpp.template = "::AlwaysThrowMap",
  ) aa_map;
  4: string (cpp.use_allocator, cpp.type = "::AlwaysThrowString") aa_string;
  5: i32 not_a_container;
  6: list<i32> not_aa_list;
  7: set<i32> not_aa_set;
  8: map<i32, i32> not_aa_map;
  9: string not_aa_string;
} (cpp.allocator = "::ScopedAlwaysThrowAlloc")

struct AlwaysThrowParent {
  1: AlwaysThrowChild (cpp.use_allocator) child;
} (cpp.allocator = "::ScopedAlwaysThrowAlloc")

struct AAStruct {
  1: i32 foo;
} (cpp.allocator = "::ScopedStatefulAlloc")

struct NoAllocatorVia {
  1: AAStruct (cpp.use_allocator) foo;
} (cpp.allocator = "::ScopedStatefulAlloc")

struct YesAllocatorVia {
  1: AAStruct (cpp.use_allocator) foo (cpp.name = "bar");
} (cpp.allocator = "::ScopedStatefulAlloc", cpp.allocator_via = "bar")

struct HasContainerFields {
  1: list<i32> (
    cpp.use_allocator,
    cpp.template = "::StatefulAllocVector",
  ) aa_list;
  2: set<i32> (cpp.use_allocator, cpp.template = "::StatefulAllocSet") aa_set;
  3: map<i32, i32> (
    cpp.use_allocator,
    cpp.template = "::StatefulAllocMap",
  ) aa_map;
} (cpp.allocator = "::ScopedStatefulAlloc")

typedef map<i32, i32> (
  cpp.use_allocator,
  cpp.template = "::StatefulAllocMap",
) StatefulAllocIntMap

typedef map<i32, StatefulAllocIntMap> (
  cpp.use_allocator,
  cpp.template = "::StatefulAllocMap",
) StatefulAllocMapMap

typedef set<i32> (
  cpp.use_allocator,
  cpp.template = "::StatefulAllocSet",
) StatefulAllocIntSet

typedef map<i32, StatefulAllocIntSet> (
  cpp.use_allocator,
  cpp.template = "::StatefulAllocMap",
) StatefulAllocMapSet

struct UsesTypedef {
  1: StatefulAllocIntMap aa_map;
} (cpp.allocator = "::ScopedStatefulAlloc", cpp.allocator_via = "aa_map")

struct HasNestedContainerFields {
  1: StatefulAllocMapMap aa_map_of_map;
  2: StatefulAllocMapSet aa_map_of_set;
} (cpp.allocator = "::ScopedStatefulAlloc")

struct HasSortedUniqueConstructibleFields {
  1: set<i32> (
    cpp.use_allocator,
    cpp.template = "::StatefulAllocSortedVectorSet",
  ) aa_set;
  2: map<i32, i32> (
    cpp.use_allocator,
    cpp.template = "::StatefulAllocSortedVectorMap",
  ) aa_map;
} (cpp.allocator = "::ScopedStatefulAlloc")

struct CountingChild {
  1: list<i32> (cpp.use_allocator, cpp.template = "::CountingVector") aa_list;
  2: set<i32> (cpp.use_allocator, cpp.template = "::CountingSet") aa_set;
  3: map<i32, i32> (cpp.use_allocator, cpp.template = "::CountingMap") aa_map;
  4: string (cpp.use_allocator, cpp.type = "::CountingString") aa_string;
  5: i32 not_a_container;
  6: list<i32> not_aa_list;
  7: set<i32> not_aa_set;
  8: map<i32, i32> not_aa_map;
  9: string not_aa_string;
} (cpp.allocator = "::ScopedCountingAlloc")

struct CountingParent {
  1: list<CountingChild> (
    cpp.use_allocator,
    cpp.template = "::CountingVector",
  ) aa_child_list;
  2: list<CountingChild> not_aa_child_list;
} (cpp.allocator = "::ScopedCountingAlloc")
