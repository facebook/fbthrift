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
