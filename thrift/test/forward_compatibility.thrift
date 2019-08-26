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
# TODO(@denpluplus, by 11/4/2017) Remove.

namespace cpp2 forward_compatibility

typedef map<i16, double> NoFCMap
typedef map<i32, float> (forward_compatibility) FCMap

struct OldStructure {
  1: NoFCMap features,
}

struct NewStructure {
  1: FCMap features,
}

struct OldStructureNested {
  1: list<NoFCMap> featuresList,
}

struct NewStructureNested {
  1: list<FCMap> featuresList,
}

typedef map<i64, double> DoubleMapType
typedef map<i16, DoubleMapType> OldMapMap
typedef map<i32, DoubleMapType>
  (forward_compatibility) NewMapMap

struct OldMapMapStruct {
  1: OldMapMap features,
}

struct NewMapMapStruct {
  1: NewMapMap features,
}

typedef map<i16, list<float>> OldMapList
typedef map<i32, list<float>>
  (forward_compatibility) NewMapList

struct OldMapListStruct {
  1: OldMapList features,
}

struct NewMapListStruct {
  1: NewMapList features,
}

service OldServer {
  OldStructure get();
}

service NewServer {
  NewStructure get();
}
