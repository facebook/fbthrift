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

include "thrift/lib/thrift/patch.thrift"

namespace cpp2 apache.thrift.test.patch

struct MyStruct {
  1: bool boolVal;
  2: byte byteVal;
  3: i16 i16Val;
  4: i32 i32Val;
  5: i64 i64Val;
  6: float floatVal;
  7: double doubleVal;
  8: string stringVal;
  9: binary (cpp.type = "::folly::IOBuf") binaryVal;
} (thrift.uri = "facebook.com/thrift/test/patch/MyStruct")

// TODO(afuller): Generate this via an annotation + ast_mutator.
struct MyStructPatch {
  1: patch.BoolPatch boolVal;
  2: patch.BytePatch byteVal;
  3: patch.I16Patch i16Val;
  4: patch.I32Patch i32Val;
  5: patch.I64Patch i64Val;
  6: patch.FloatPatch floatVal;
  7: patch.DoublePatch doubleVal;
  8: patch.StringPatch stringVal;
  9: patch.BinaryPatch binaryVal;
} (thrift.uri = "facebook.com/thrift/test/patch/MyStructPatch")

// TODO(afuller): Generate this via an annotation + ast_mutator (though
// eventually it should be a shared type-parameterized struct).
struct MyStructValuePatch {
  // Assigns to a given struct. If set, all other operations are ignored.
  1: optional MyStruct assign (thrift.box);

  // Clears a given struct.
  2: bool clear;
} (thrift.uri = "facebook.com/thrift/test/patch/MyStructValuePatch")
