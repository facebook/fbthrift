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

namespace cpp2 test_cpp2.cpp_reflection_no_metadata

enum enum1 {
  field0 = 0,
  field1 = 1,
  field2 = 2,
}

enum enum2 {
  field0_2 = 0,
  field1_2 = 1,
  field2_2 = 2,
}

union union1 {
  1: i32 ui
  2: double ud
  3: string us
  4: enum1 ue
}

union union2 {
  1: i32 ui_2
  2: double ud_2
  3: string us_2
  4: enum1 ue_2
}

struct struct1 {
  1: required i32 field0
  2: optional string field1
  3: enum1 field2
  4: required enum2 field3
  5: optional union1 field4
  6: union2 field5
}
