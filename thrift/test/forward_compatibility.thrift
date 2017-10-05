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

service OldServer {
  OldStructure get();
}

service NewServer {
  NewStructure get();
}
