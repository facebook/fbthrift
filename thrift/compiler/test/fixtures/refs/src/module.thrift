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

namespace java.swift test.fixtures.refs

union MyUnion {
  1: i32 anInteger,
  2: string aString,
}

struct MyField {
  1: optional i64 opt_value,
  2: i64 value,
  3: required i64 req_value,
}

struct MyStruct {
  1: optional MyField opt_ref (cpp.ref = "true", cpp2.ref = "true")
  2: MyField ref (cpp.ref = "true", cpp2.ref = "true")
  3: required MyField req_ref (cpp.ref = "true", cpp2.ref = "true")
}

struct StructWithUnion {
  1: MyUnion u (cpp.ref = "true"),
  2: double aDouble,
  3: MyField f,
}

struct RecursiveStruct {
  1: optional list<RecursiveStruct> mes (swift.recursive_reference = "true"),
}

struct StructWithContainers {
  1: list<i32> list_ref (cpp.ref = "true", cpp2.ref = "true")
  2: set<i32> set_ref (cpp.ref = "true", cpp2.ref = "true")
  3: map<i32, i32> map_ref (cpp.ref = "true", cpp2.ref = "true")
  4: list<i32> list_ref_unique
      (cpp.ref_type = "unique", cpp2.ref_type = "unique")
  5: set<i32> set_ref_shared (cpp.ref_type = "shared", cpp2.ref_type = "shared")
  6: list<i32> list_ref_shared_const
      (cpp.ref_type = "shared_const", cpp2.ref_type = "shared_const")
}

struct StructWithSharedConst {
  1: optional MyField opt_shared_const
      (cpp.ref_type = "shared_const", cpp2.ref_type = "shared_const")
  2: MyField shared_const
      (cpp.ref_type = "shared_const", cpp2.ref_type = "shared_const")
  3: required MyField req_shared_const
      (cpp.ref_type = "shared_const", cpp2.ref_type = "shared_const")
}

enum TypedEnum {
  VAL1 = 0,
  VAL2 = 1,
} (cpp.enum_type = "short")

struct Empty {}

struct StructWithRef {
  1:          Empty def_field (cpp.ref),
  2: optional Empty opt_field (cpp.ref),
  3: required Empty req_field (cpp.ref),
}

const StructWithRef kStructWithRef = {
  "def_field": {},
  "opt_field": {},
  "req_field": {},
}

struct StructWithRefTypeUnique {
  1:          Empty def_field (cpp.ref_type = "unique"),
  2: optional Empty opt_field (cpp.ref_type = "unique"),
  3: required Empty req_field (cpp.ref_type = "unique"),
}

const StructWithRefTypeUnique kStructWithRefTypeUnique = {
  "def_field": {},
  "opt_field": {},
  "req_field": {},
}

struct StructWithRefTypeShared {
  1:          Empty def_field (cpp.ref_type = "shared"),
  2: optional Empty opt_field (cpp.ref_type = "shared"),
  3: required Empty req_field (cpp.ref_type = "shared"),
}

const StructWithRefTypeShared kStructWithRefTypeShared = {
  "def_field": {},
  "opt_field": {},
  "req_field": {},
}

struct StructWithRefTypeSharedConst {
  1:          Empty def_field (cpp.ref_type = "shared_const"),
  2: optional Empty opt_field (cpp.ref_type = "shared_const"),
  3: required Empty req_field (cpp.ref_type = "shared_const"),
}

const StructWithRefTypeSharedConst kStructWithRefTypeSharedConst = {
  "def_field": {},
  "opt_field": {},
  "req_field": {},
}

struct StructWithRefAndAnnotCppNoexceptMoveCtor {
  1: Empty def_field (cpp.ref),
} (cpp.noexcept_move_ctor)
