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

// Fixture types for test_serialization.py. The generated thrift-python
// classes are the oracle; the same types, read back from the SchemaRegistry,
// drive the codec under test.

include "thrift/annotation/thrift.thrift"
include "thrift/lib/thrift/any.thrift"

package "thrift.com/python/schema/codec_test"

namespace py3 thrift.lib.python.schema.tests

enum Color {
  RED = 0,
  GREEN = 1,
  BLUE = 2,
}

struct Primitives {
  1: bool bool_field;
  2: byte byte_field;
  3: i16 i16_field;
  4: i32 i32_field;
  5: i64 i64_field;
  6: float float_field;
  7: double double_field;
  8: string string_field;
  9: binary binary_field;
  10: Color enum_field;
}

struct Point {
  1: i32 x;
  2: i32 y;
}

union Choice {
  1: i32 number;
  2: string text;
  3: Point point;
  4: list<bool> flags;
}

exception Failure {
  1: string reason;
  2: i32 code;
}

struct Containers {
  1: list<i32> ints;
  2: list<bool> bools;
  3: set<string> names;
  4: map<string, i64> counts;
  5: list<list<i32>> nested_lists;
  6: map<i32, list<string>> lists_by_id;
  7: list<map<string, set<i16>>> deep;
  // @lint-ignore THRIFTCHECKS bad-key-type
  8: set<Point> points;
  // @lint-ignore THRIFTCHECKS bad-key-type
  9: map<Point, string> labels;
  10: list<Choice> choices;
  11: map<Color, double> weights;
  12: list<float> floats;
  13: map<binary, bool> flags_by_blob;
  // @lint-ignore THRIFTCHECKS bad-key-type
  14: map<bool, byte> bytes_by_flag;
  // @lint-ignore THRIFTCHECKS bad-key-type
  15: map<double, string> names_by_weight;
  16: list<binary> blobs;
  17: list<Color> colors;
  18: list<Failure> failures;
}

struct Presence {
  1: i32 plain;
  2: optional i32 maybe;
  @thrift.TerseWrite
  3: i32 terse;
  4: i32 plain_default = 7;
  5: optional string maybe_string;
  @thrift.TerseWrite
  6: string terse_string;
  7: list<i32> ints_default = [1, 2, 3];
  8: Point point_default = Point{x = 1, y = 2};
  9: Choice choice_default = Choice{text = "hi"};
  10: optional Point maybe_point;
  11: Color color_default = Color.BLUE;
  12: map<string, i32> map_default = {"a": 1};
  13: set<i32> set_default = [4, 5];
  14: double double_default = 2.5;
  15: binary binary_default = "raw";
  16: float float_default = 0.5;
  17: bool bool_default = true;
}

// Declared out of id order, with gaps over 15 and a negative id, so the
// compact protocol needs long field headers.
struct FieldIds {
  1: i32 one;
  17: i32 seventeen;
  2: i32 two;
  -5: i32 negative;
  300: bool far_bool;
  32767: i64 max_id;
}

struct WithAny {
  1: i32 id;
  2: any.Any payload;
  3: optional any.Any maybe;
}

struct Nested {
  1: Primitives primitives;
  2: Containers containers;
  @thrift.Box
  3: optional Nested next;
  4: Failure failure;
  5: Choice choice;
}

// A struct with only a bool field and a list of more than 14 bools.
struct Flags {
  1: bool flag;
  2: list<bool> many;
}

// Empty unions that C++ keeps apart from absent fields: held by an optional
// field and by a union field.
union Wrapper {
  1: Choice choice;
  2: i32 number;
}

struct EmptyUnions {
  1: optional Choice maybe_choice;
  2: Wrapper wrapper;
}
