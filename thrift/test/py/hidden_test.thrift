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

include "thrift/annotation/python.thrift"

namespace py test.fixtures.hidden_test
namespace py3 test.fixtures

struct StructuredKey {
  1: i32 part1;
  2: string part2;
}

struct S1 {
  1: i32 normalField;

  @python.PyDeprecatedHidden{reason = "Structured keys not supported"}
  2: map<StructuredKey, i32> mapField;
}