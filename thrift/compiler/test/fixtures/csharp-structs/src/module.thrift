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

// Minimal fixture for testing C# struct code generation.
// This fixture intentionally uses simple types to test struct codegen
// without dependencies on union codegen. Delete this fixture once the
// full cross-language fixtures compile cleanly.

package "test.dev/fixtures/csharp_structs"

namespace csharp Test.Fixtures.CsharpStructs

// Basic struct
struct Point {
  1: i32 x;
  2: i32 y;
}

// Struct with default values
struct Person {
  1: string name = "Unknown";
  2: i32 age = 0;
  3: optional string email;
}

// Struct with collections
struct Container {
  1: list<i32> items;
  2: map<string, i32> lookup;
  3: set<string> tags;
}

// Nested struct
struct Rectangle {
  1: Point topLeft;
  2: Point bottomRight;
}

// Enum for struct field
enum Status {
  ACTIVE = 0,
  INACTIVE = 1,
  PENDING = 2,
}

// Struct with enum field
struct Task {
  1: string name;
  2: Status status = Status.PENDING;
}

// Struct with all primitive types
struct AllPrimitives {
  1: bool boolField;
  2: byte byteField;
  3: i16 shortField;
  4: i32 intField;
  5: i64 longField;
  6: double doubleField;
  7: string stringField;
}

// ============================================================================
// Struct constants for testing struct constant code generation
// ============================================================================

const Point ORIGIN = Point{x = 0, y = 0};
const Point UNIT_POINT = Point{x = 1, y = 1};
const Person DEFAULT_PERSON = Person{name = "John Doe", age = 30};
const Task SAMPLE_TASK = Task{name = "Sample Task", status = Status.ACTIVE};

// ============================================================================
// Union definitions for testing union code generation
// ============================================================================

// Basic union with primitive types
union SimpleUnion {
  1: i32 intValue;
  2: string stringValue;
  3: bool boolValue;
}

// Union with different field types including collections
union TypesUnion {
  1: i64 longValue;
  2: double doubleValue;
  3: list<i32> listValue;
  4: map<string, i32> mapValue;
}

// Union with struct field
union StructUnion {
  1: Point pointValue;
  2: Person personValue;
}

// Union with enum field
union EnumUnion {
  1: Status statusValue;
  2: i32 intValue;
}
