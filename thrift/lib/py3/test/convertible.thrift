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

enum Color {
  NONE = 0,
  RED = 1,
  GREEN = 2,
  BLUE = 3,
}

struct Simple {
  1: i32 intField,
  2: string strField,
  3: list<i32> intList,
  4: set<string> strSet,
  5: map<string, i64> strToIntMap,
  6: Color color,
}

struct Nested {
  1: Simple simpleField,
  2: list<Simple> simpleList,
  3: map<Color, Simple> colorToSimpleMap,
}

union Union {
  1: i32 intField,
  2: string strField,
  3: list<i32> intList,
  4: Simple simpleField,
}
