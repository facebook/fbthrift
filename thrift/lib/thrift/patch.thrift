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

// TODO(afuller): Re-enable when supported by hs2
// @thrift.v1alpha
package "facebook.com/thrift/op"

namespace cpp2 apache.thrift.op
namespace py3 apache.thrift.op
namespace java com.facebook.thrift.op
namespace java.swift com.facebook.thrift.op_swift
namespace java2 com.facebook.thrift.op
namespace py.asyncio apache_thrift_asyncio.patch
namespace go thrift.lib.thrift.patch
namespace py thrift.lib.thrift.patch

// An annotation that indicates a patch representation
// should be generated for the associated definition.
@scope.Program
@scope.Structured
struct GeneratePatch {}

@scope.Struct
struct GenerateOptionalPatch {}

// A patch for a boolean value.
@GenerateOptionalPatch
struct BoolPatch {
  // Assign to a given value.
  //
  // If set, all other patch operations are ignored.
  1: optional bool assign;

  // If the bool value should be inverted.
  5: bool invert;
} (
  cpp.detail.underlying_name = "BoolPatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::BoolPatchAdapter",
)

// A patch for an 8-bit integer value.
@GenerateOptionalPatch
struct BytePatch {
  // Assign to a given value.
  //
  // If set, all other patch operations are ignored.
  1: optional byte assign;

  // Add to a given value.
  5: byte add;
} (
  cpp.detail.underlying_name = "BytePatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::NumberPatchAdapter",
)

// A patch for a 16-bit integer value.
@GenerateOptionalPatch
struct I16Patch {
  // Assign to a given value.
  //
  // If set, all other patch operations are ignored.
  1: optional i16 assign;

  // Add to a given value.
  5: i16 add;
} (
  cpp.detail.underlying_name = "I16PatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::NumberPatchAdapter",
)

// A patch for a 32-bit integer value.
@GenerateOptionalPatch
struct I32Patch {
  // Assign to a given value.
  //
  // If set, all other patch operations are ignored.
  1: optional i32 assign;

  // Add to a given value.
  5: i32 add;
} (
  cpp.detail.underlying_name = "I32PatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::NumberPatchAdapter",
)

// A patch for a 64-bit integer value.
@GenerateOptionalPatch
struct I64Patch {
  // Assign to a given value.
  //
  // If set, all other patch operations are ignored.
  1: optional i64 assign;

  // Add to a given value.
  5: i64 add;
} (
  cpp.detail.underlying_name = "I64PatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::NumberPatchAdapter",
)

// A patch for a 32-bit floating point value.
@GenerateOptionalPatch
struct FloatPatch {
  // Assign to a given value.
  //
  // If set, all other patch operations are ignored.
  1: optional float assign;

  // Add to a given value.
  5: float add;
} (
  cpp.detail.underlying_name = "FloatPatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::NumberPatchAdapter",
)

// A patch for an 64-bit floating point value.
@GenerateOptionalPatch
struct DoublePatch {
  // Assign to a given value.
  //
  // If set, all other patch operations are ignored.
  1: optional double assign;

  // Add to a given value.
  5: double add;
} (
  cpp.detail.underlying_name = "DoublePatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::NumberPatchAdapter",
)

// A patch for a string value.
@GenerateOptionalPatch
struct StringPatch {
  // Assign to a given value.
  //
  // If set, all other patch operations are ignored.
  1: optional string assign;

  // Clear a given string.
  2: bool clear;

  // Prepend to a given value.
  4: string prepend;

  // Append to a given value.
  5: string append;
} (
  cpp.detail.underlying_name = "StringPatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::StringPatchAdapter",
)

// A patch for a binary value.
@GenerateOptionalPatch
struct BinaryPatch {
  // Assign to a given value.
  //
  // If set, all other patch operations are ignored.
  1: optional binary (cpp.type = "::folly::IOBuf") assign;
} (
  cpp.detail.underlying_name = "BinaryPatchStruct",
  cpp.adapter = "::apache::thrift::op::detail::AssignPatchAdapter",
)
