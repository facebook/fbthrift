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

struct Primitives {
  1: i32 f1;
  2: i64 f2;
  3: i64 f3;
  4: i32 f4;
  5: string f5;
  6: string f6;
  7: string f7;
  8: float f8;
  9: double f9;
}

struct PrimitivesSimpleSkip {
  1: i32 f1;
  2: i64 f2;
  // Altered
  10: i64 f3;
  4: i32 f4;
  // Altered
  11: string f5;
  6: string f6;
  // Missing
}

struct PrimitivesConsecutiveMissing {
  1: i32 f1;
  // Missing
  6: string f6;
  // Missing
}

struct PrimitivesTypesChanged {
  // Altered
  1: i64 f1;
  2: i64 f2;
  // Altered
  3: i32 f3;
  4: i32 f4;
  // Altered
  5: double f5;
  // Altered
  6: list<i32> f6;
  7: string f7;
  8: float f8;
  // Altered
  9: float f9;
}

struct PrimitivesTypesReordered {
  5: string f5;
  3: i64 f3;
  2: i64 f2;
  1: i32 f1;
  9: double f9;
  4: i32 f4;
  7: string f7;
  6: string f6;
  8: float f8;
}

struct BigFieldIds {
  1: i32 f1;
  100: i32 f100;
  2: i32 f2;
  101: i32 f101;
  102: i32 f102;
  1000: i32 f1000;
  1001: i32 f1001;
  3: i32 f3;
  4: i32 f4;
}

struct BigFieldIdsMissing {
  1: i32 f1;
  2: i32 f2;
  4: i32 f4;
}

struct NestedStructBase {
  1: i32 f1;
  2: i32 f2;
}

struct NestedStructL1 {
  1: i32 f1;
  2: i32 f2;
  3: NestedStructBase f3;
}

struct NestedStructL2 {
  1: string f1;
  2: NestedStructL1 f2;
  3: i32 f3;
}

struct NestedStructMissingSubstruct {
  1: string f1;
  3: i32 f3;
}

struct NestedStructTypeChanged {
  1: string f1;
  2: float f2;
  3: i32 f3;
}
