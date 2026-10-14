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

package "thrift.com/python/test/reflection/container_union"

namespace py3 python_test.reflection

union Choice {
  1: i32 number;
  2: string text;
}

struct Item {
  1: i32 id;
}

struct Containers {
  1: list<Choice> choice_list;
  2: set<i32> ids;
  3: map<string, Choice> choice_map;
  4: map<i32, list<Choice>> nested_choices;
  5: list<Item> item_list;
}
