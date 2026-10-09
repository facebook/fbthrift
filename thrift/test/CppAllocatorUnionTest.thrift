/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

include "thrift/annotation/cpp.thrift"
include "thrift/annotation/thrift.thrift"

package "facebook.com/thrift/test/cpp_allocator_union"

namespace cpp2 apache.thrift.test

cpp_include "thrift/test/CppAllocatorTest.h"

@thrift.DeprecatedUnvalidatedAnnotations{
  items = {"cpp.allocator": "::ScopedCountingAlloc<>"},
}
@cpp.InternalExperimentalAllowAllocatorOnUnion
union CountingUnion {
  1: i32 n;
}

@thrift.DeprecatedUnvalidatedAnnotations{items = {"cpp.use_allocator": "1"}}
@cpp.Type{name = "std::pmr::string"}
typedef string UPmrString

// No cpp.use_allocator: never takes the union's allocator.
@cpp.Type{name = "std::pmr::string"}
typedef string NotAAPmrString

@thrift.DeprecatedUnvalidatedAnnotations{
  items = {"cpp.allocator": "PmrByteAlloc"},
}
@cpp.InternalExperimentalAllowAllocatorOnUnion
union UnionPmr {
  1: UPmrString aa_string;
  6: i32 not_a_container;
  7: string not_aa_string;
  8: NotAAPmrString not_aa_pmr_string;
}
@thrift.DeprecatedUnvalidatedAnnotations{items = {"cpp.use_allocator": "1"}}
typedef UnionPmr UnionPmrAA
@thrift.DeprecatedUnvalidatedAnnotations{items = {"cpp.use_allocator": "1"}}
@cpp.Type{template = "std::pmr::vector"}
typedef list<UnionPmr> UPmrUnionList

@thrift.DeprecatedUnvalidatedAnnotations{
  items = {"cpp.allocator": "PmrByteAlloc"},
}
struct HasUnionPmr {
  1: UnionPmrAA u;
  2: UPmrUnionList us;
  // No cpp.use_allocator: on the default resource, like a struct field.
  3: UnionPmr heap_u;
}

@thrift.DeprecatedUnvalidatedAnnotations{
  items = {"cpp.allocator": "PmrByteAlloc"},
}
@cpp.InternalExperimentalAllowAllocatorOnUnion
union RefChildUnionPmr {
  @cpp.Ref{type = cpp.RefType.Unique}
  2: string ref_s;
}

@thrift.DeprecatedUnvalidatedAnnotations{
  items = {"cpp.allocator": "PmrByteAlloc", "cpp.noncopyable": "1"},
}
@cpp.InternalExperimentalAllowAllocatorOnUnion
union NoncopyableUnionPmr {
  1: NotAAPmrString s;
}

@thrift.DeprecatedUnvalidatedAnnotations{
  items = {"cpp.allocator": "PmrByteAlloc"},
}
@cpp.InternalExperimentalAllowAllocatorOnUnion
union EmptyUnionPmr {}
