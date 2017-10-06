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
