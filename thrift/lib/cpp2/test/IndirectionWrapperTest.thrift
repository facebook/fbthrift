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

cpp_include "thrift/lib/cpp2/test/IndirectionWrapperTest.h"

namespace cpp2 apache.thrift.test

struct Foo_data {
  1: optional Bar bar (cpp.ref = 'true');
  2: i32 field;
}

typedef Foo_data (cpp.type = 'Foo', cpp.indirection) Foo
typedef i64 (cpp.type = "Seconds", cpp.indirection) Seconds

struct Bar_data {
  1: Foo foo;
  2: i32 field;
  3: Seconds seconds;
}

typedef Bar_data (cpp.type = 'Bar', cpp.indirection) Bar
