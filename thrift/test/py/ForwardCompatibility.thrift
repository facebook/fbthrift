namespace py ForwardCompatibility.ForwardCompatibility

typedef map<i16, double> NoFCMap

struct OldStructure {
    1: NoFCMap features,
}

typedef map<i32, float> (forward_compatibility) FCMap

struct NewStructure {
    1: FCMap features,
}

struct OldStructureNested {
  1: OldStructureNested dummy,
  2: list<NoFCMap> features,
}

struct NewStructureNested {
  1: NewStructureNested dummy,
  2: list<FCMap> features,
}
