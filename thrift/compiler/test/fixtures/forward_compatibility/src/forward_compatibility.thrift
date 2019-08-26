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
struct OldStructure {
  1: map<i16, double> features
}

struct NewStructure {
  1: map<i16, double> (forward_compatibility) features
}

typedef map<i16, float> (forward_compatibility) FloatFeatures

struct NewStructure2 {
  1: FloatFeatures features
}

struct NewStructureNested {
  1: list<FloatFeatures> lst,
  2: map<i16, FloatFeatures> mp,
  3: set<FloatFeatures> s,
}

struct NewStructureNestedField {
  1: NewStructureNested f,
}

typedef map<i64, double> DoubleMapType
typedef map<i16, DoubleMapType> OldMapMap
typedef map<i32, DoubleMapType>
  (forward_compatibility) NewMapMap

typedef map<i16, list<float>> OldMapList
typedef map<i32, list<float>>
  (forward_compatibility) NewMapList
