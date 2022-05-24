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

struct SimpleStruct1 {
  1: i32 i1;
  2: i64 i2;
  3: float f1;
  4: double f2;
}

struct ComplexStruct {
  1: list<string> l;
  2: set<string> s;
  3: map<string, string> m;
  4: map<string, list<i32>> ml;
  5: map<string, map<i32, i32>> mm;
}
