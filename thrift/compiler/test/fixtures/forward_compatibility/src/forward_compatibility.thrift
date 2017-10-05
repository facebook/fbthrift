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
