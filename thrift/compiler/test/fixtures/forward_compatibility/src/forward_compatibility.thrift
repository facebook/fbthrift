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
