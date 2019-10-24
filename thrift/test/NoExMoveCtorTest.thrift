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

cpp_include "thrift/test/NoExMoveCtorTest.h"

struct Simple {
  1: bool a,
  2: i32 b,
  3: double c,
  4: string d,
  5: set<i32> e,
  6: list<float> f,
  7: map<i32, string> g,
  8: map<string, string> h,
  9: optional bool i,
} (cpp.noexcept_move_ctor, not.used.key = "not.used.value")


typedef map<string, string> ( cpp.type = "s2sumap" ) mapx

struct Complex {
  1: Simple s1,
  2: list<Simple> s2,
  3: map<i32, Simple> s3,
  4: mapx m,
}

struct ComplexEx {
  1: Simple s1,
  2: list<Simple> s2,
  3: map<i32, Simple> s3,
  4: mapx m,
} (cpp.noexcept_move_ctor, not.used.key = "not.used.value")


typedef string (cpp.type = "ThrowCtorType") TThrowCtorType

struct MayThrowInDefMoveCtorStruct {
  1: string a,
  2: TThrowCtorType b,
}

struct MayThrowInDefMoveCtorStructEx {
  1: string a,
  2: TThrowCtorType b,
} (cpp.noexcept_move_ctor, not.used.key = "not.used.value")
