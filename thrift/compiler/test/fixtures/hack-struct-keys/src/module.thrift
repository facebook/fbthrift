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

struct Foo {
  1: i32 fiels;
}

struct Bar {
 1: set<Foo> a;
 2: map<Foo, i32> b;
}

service Baz {
  string qux(
    1: set<Foo> a,
    2: list<Bar> b,
    3: map<Foo, string> c,
  );
}
