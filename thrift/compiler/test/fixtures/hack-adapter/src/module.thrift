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

typedef set<string> (hack.adapter = '\Adapter2') SetWithAdapter
typedef list<string (hack.adapter = '\Adapter1')> ListWithElemAdapter

struct Foo {
  1: i32 (hack.adapter = '\Adapter1') intField;
  2: optional i32 (hack.adapter = '\Adapter1') optionalIntField;
  3: SetWithAdapter setField;
  4: optional SetWithAdapter optionalSetField;
  5: map<string, ListWithElemAdapter (hack.adapter = '\Adapter2')> (
    hack.adapter = '\Adapter3',
  ) mapField;
  6: optional map<string, ListWithElemAdapter (hack.adapter = '\Adapter2')> (
    hack.adapter = '\Adapter3',
  ) optionalMapField;
}

struct Bar {
  1: Foo (hack.adapter = '\Adapter1') structField;
  2: optional Foo (hack.adapter = '\Adapter1') optionalStructField;
  3: list<Foo (hack.adapter = '\Adapter1')> structListField;
  4: optional list<Foo (hack.adapter = '\Adapter1')> optionalStructListField;
}
