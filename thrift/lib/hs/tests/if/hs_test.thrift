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

struct Foo {
  5: i32 bar,
  1: i32 baz,
}

struct TestStruct {
  1: required bool f_bool,
  2: byte f_byte,
  3: double f_double,
  4: i16 f_i16 = 5,
  5: i32 f_i32,
  6: i64 f_i64,
  7: float f_float,
  8: list<i16> f_list,
  9: map<i16,i32> f_map = {1:2},
  10: string f_string,
  11: set<byte> f_set,
  12: optional i32 o_i32,
  99: Foo foo = {"bar":1,"baz":2},
}
