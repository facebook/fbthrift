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

namespace cpp2 cpp2

struct RecursiveStruct {
  1:          RecursiveStruct def_field (cpp2.ref = "true")
  2: optional RecursiveStruct opt_field (cpp2.ref = "true")
  3: required RecursiveStruct req_field (cpp2.ref = "true")
}

struct PlainStruct {
  1: i32 field
}

struct ReferringStruct {
  1:           PlainStruct def_field (cpp2.ref = "true")
  2:  optional PlainStruct opt_field (cpp2.ref = "true")
  3:  required PlainStruct req_field (cpp2.ref = "true")
  4:           PlainStruct def_unique_field (cpp2.ref_type = "unique")
  5:  optional PlainStruct opt_unique_field (cpp2.ref_type = "unique")
  6:  required PlainStruct req_unique_field (cpp2.ref_type = "unique")
  7:           PlainStruct def_shared_field (cpp2.ref_type = "shared")
  8:  optional PlainStruct opt_shared_field (cpp2.ref_type = "shared")
  9:  required PlainStruct req_shared_field (cpp2.ref_type = "shared")
  10:          PlainStruct def_shared_const_field
               (cpp2.ref_type = "shared_const")
  11: optional PlainStruct opt_shared_const_field
               (cpp2.ref_type = "shared_const")
  12: required PlainStruct req_shared_const_field
               (cpp2.ref_type = "shared_const")
}

struct StructWithContainers {
  1:           list<i32> def_list_ref (cpp.ref = "true", cpp2.ref = "true")
  2:           set<i32> def_set_ref (cpp.ref = "true", cpp2.ref = "true")
  3:           map<i32, i32> def_map_ref (cpp.ref = "true", cpp2.ref = "true")
  4:           list<i32> def_list_ref_unique
               (cpp.ref_type = "unique", cpp2.ref_type = "unique")
  5:           set<i32> def_set_ref_shared
               (cpp.ref_type = "shared", cpp2.ref_type = "shared")
  6:           list<i32> def_list_ref_shared_const
               (cpp.ref_type = "shared_const", cpp2.ref_type = "shared_const")
  7:  required list<i32> req_list_ref (cpp.ref = "true", cpp2.ref = "true")
  8:  required set<i32> req_set_ref (cpp.ref = "true", cpp2.ref = "true")
  9:  required map<i32, i32> req_map_ref (cpp.ref = "true", cpp2.ref = "true")
  10: required list<i32> req_list_ref_unique
               (cpp.ref_type = "unique", cpp2.ref_type = "unique")
  11: required set<i32> req_set_ref_shared
               (cpp.ref_type = "shared", cpp2.ref_type = "shared")
  12: required list<i32> req_list_ref_shared_const
               (cpp.ref_type = "shared_const", cpp2.ref_type = "shared_const")
  13: optional list<i32> opt_list_ref (cpp.ref = "true", cpp2.ref = "true")
  14: optional set<i32> opt_set_ref (cpp.ref = "true", cpp2.ref = "true")
  15: optional map<i32, i32> opt_map_ref (cpp.ref = "true", cpp2.ref = "true")
  16: optional list<i32> opt_list_ref_unique
               (cpp.ref_type = "unique", cpp2.ref_type = "unique")
  17: optional set<i32> opt_set_ref_shared
               (cpp.ref_type = "shared", cpp2.ref_type = "shared")
  18: optional list<i32> opt_list_ref_shared_const
               (cpp.ref_type = "shared_const", cpp2.ref_type = "shared_const")
}
