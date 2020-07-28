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

struct NonstandardCollectionTypes {
  10: set<string> defaultset;
  11: set<string> (rust.type = "BTreeSet") btreeset;
  12: set<string> (rust.type = "HashSet") hashset;
  13: set<string> (rust.type = "indexmap::IndexSet") indexset_a;
  14: set<string> (rust.type = "indexmap::IndexSet") indexset_b;
  15: set<i64> (rust.type = "indexmap::IndexSet") indexset_c;

  20: map<string, string> defaultmap;
  21: map<string, string> (rust.type = "BTreeMap") btreemap;
  22: map<string, string> (rust.type = "HashMap") hashmap;
  23: map<string, string> (rust.type = "indexmap::IndexMap") indexmap_a;
  24: map<string, string> (rust.type = "indexmap::IndexMap") indexmap_b;
  25: map<string, i64> (rust.type = "indexmap::IndexMap") indexmap_c;
}

enum TestEnum {
  FOO = 1
  BAR = 2
  BAZ = 3
}

enum TestEnumEmpty {
}
