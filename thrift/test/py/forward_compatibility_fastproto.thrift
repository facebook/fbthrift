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
