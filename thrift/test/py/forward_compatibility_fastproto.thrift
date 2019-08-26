/*
 * Copyright 2019-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
namespace py forward_compatibility_fastproto

typedef map<i16, double> NoFCMap

struct OldStructure {
  1: NoFCMap features,
}

typedef map<i32, float> (forward_compatibility) FCMap

struct NewStructure {
  1: FCMap features,
}

struct OldStructureNested {
  1: list<NoFCMap> features,
}

struct NewStructureNested {
  1: list<FCMap> features,
}

struct OldStructureNestedNested {
  1: OldStructureNested field,
}

struct NewStructureNestedNested {
  1: NewStructureNested field,
}

struct A {
  1: A field,
}
