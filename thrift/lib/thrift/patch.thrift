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
include "thrift/annotation/scope.thrift"
cpp_include "thrift/lib/cpp2/op/detail/Patch.h"

namespace cpp2 apache.thrift.op
namespace java.swift com.facebook.thrift.op
namespace java2 com.facebook.thrift.op

// An annotation that indicates a patch representation
// should be generated for the associated definition.
@scope.Struct
@thrift.Experimental
struct GeneratePatch {} (thrift.uri = "facebook.com/thrift/op/GeneratePatch")

@scope.Struct
@thrift.Experimental
struct GenerateOptionalPatch {} (
  thrift.uri = "facebook.com/thrift/op/GenerateOptionalPatch",
)

// A patch for a boolean value.
@GenerateOptionalPatch
@thrift.Experimental
struct BoolPatch {
  // Assign to a given value.
  //
  // If set, all other patch operations are ignored.
  1: optional bool assign;

  // If the bool value should be inverted.
  2: bool invert;
} (
  cpp.name = "BoolPatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::BoolPatchAdapter",
)

// A patch for an 8-bit integer value.
@GenerateOptionalPatch
@thrift.Experimental
struct BytePatch {
  // Assign to a given value.
  //
  // If set, all other patch operations are ignored.
  1: optional byte assign;

  // Add to a given value.
  2: byte add;
} (
  cpp.name = "BytePatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::NumberPatchAdapter",
)

// A patch for a 16-bit integer value.
@GenerateOptionalPatch
@thrift.Experimental
struct I16Patch {
  // Assign to a given value.
  //
  // If set, all other patch operations are ignored.
  1: optional i16 assign;

  // Add to a given value.
  2: i16 add;
} (
  cpp.name = "I16PatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::NumberPatchAdapter",
)

// A patch for a 32-bit integer value.
@GenerateOptionalPatch
@thrift.Experimental
struct I32Patch {
  // Assign to a given value.
  //
  // If set, all other patch operations are ignored.
  1: optional i32 assign;

  // Add to a given value.
  2: i32 add;
} (
  cpp.name = "I32PatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::NumberPatchAdapter",
)

// A patch for a 64-bit integer value.
@GenerateOptionalPatch
@thrift.Experimental
struct I64Patch {
  // Assign to a given value.
  //
  // If set, all other patch operations are ignored.
  1: optional i64 assign;

  // Add to a given value.
  2: i64 add;
} (
  cpp.name = "I64PatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::NumberPatchAdapter",
)

// A patch for a 32-bit floating point value.
@GenerateOptionalPatch
@thrift.Experimental
struct FloatPatch {
  // Assign to a given value.
  //
  // If set, all other patch operations are ignored.
  1: optional float assign;

  // Add to a given value.
  2: float add;
} (
  cpp.name = "FloatPatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::NumberPatchAdapter",
)

// A patch for an 64-bit floating point value.
@GenerateOptionalPatch
@thrift.Experimental
struct DoublePatch {
  // Assign to a given value.
  //
  // If set, all other patch operations are ignored.
  1: optional double assign;

  // Add to a given value.
  2: double add;
} (
  cpp.name = "DoublePatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::NumberPatchAdapter",
)

// A patch for a string value.
@GenerateOptionalPatch
@thrift.Experimental
struct StringPatch {
  // Assign to a given value.
  //
  // If set, all other patch operations are ignored.
  1: optional string assign;

  // Append to a given value.
  4: string append;

  // Prepend to a given value.
  5: string prepend;
} (
  cpp.name = "StringPatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::StringPatchAdapter",
)

// A patch for a binary value.
@GenerateOptionalPatch
@thrift.Experimental
struct BinaryPatch {
  // Assign to a given value.
  //
  // If set, all other patch operations are ignored.
  1: optional binary (cpp.type = "::folly::IOBuf") assign;
} (
  cpp.name = "BinaryPatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::AssignPatchAdapter",
)
