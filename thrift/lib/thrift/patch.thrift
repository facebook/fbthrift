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

include "thrift/annotation/thrift.thrift"
cpp_include "thrift/lib/cpp2/op/detail/Patch.h"

namespace cpp2 apache.thrift.op

// A patch for a boolean value.
@thrift.Experimental
struct BoolPatch {
  // Assign to the given value.
  //
  // If set, all other patch operations are ignored.
  1: optional bool assign;
} (
  cpp.name = "BoolPatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::BoolPatchAdapter",
)

// A patch for an 8-bit integer value.
@thrift.Experimental
struct BytePatch {
  // Assign to the given value.
  //
  // If set, all other patch operations are ignored.
  1: optional byte assign;
} (
  cpp.name = "BytePatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::NumberPatchAdapter",
)

// A patch for a 16-bit integer value.
@thrift.Experimental
struct I16Patch {
  // Assign to the given value.
  // If set, all other patch operations are ignored.
  1: optional i16 assign;
} (
  cpp.name = "I16PatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::NumberPatchAdapter",
)

// A patch for a 32-bit integer value.
@thrift.Experimental
struct I32Patch {
  // Assign to the given value.
  //
  // If set, all other patch operations are ignored.
  1: optional i32 assign;
} (
  cpp.name = "I32PatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::NumberPatchAdapter",
)

// A patch for a 64-bit integer value.
@thrift.Experimental
struct I64Patch {
  // Assign to the given value.
  //
  // If set, all other patch operations are ignored.
  1: optional i64 assign;
} (
  cpp.name = "I64PatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::NumberPatchAdapter",
)

// A patch for a 32-bit floating point value.
@thrift.Experimental
struct FloatPatch {
  // Assign to the given value.
  //
  // If set, all other patch operations are ignored.
  1: optional float assign;
} (
  cpp.name = "FloatPatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::NumberPatchAdapter",
)

// A patch for an 64-bit floating point value.
@thrift.Experimental
struct DoublePatch {
  // Assign to the given value.
  //
  // If set, all other patch operations are ignored.
  1: optional double assign;
} (
  cpp.name = "DoublePatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::NumberPatchAdapter",
)

// A patch for a string value.
@thrift.Experimental
struct StringPatch {
  // Assign to the given value.
  //
  // If set, all other patch operations are ignored.
  1: optional string assign;
} (
  cpp.name = "StringPatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::StringPatchAdapter",
)

// A patch for a binary value.
@thrift.Experimental
struct BinaryPatch {
  // Assign to the given value.
  //
  // If set, all other patch operations are ignored.
  1: optional binary (cpp.type = "::folly::IOBuf") assign;
} (
  cpp.name = "BinaryPatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::BinaryPatchAdapter",
)
