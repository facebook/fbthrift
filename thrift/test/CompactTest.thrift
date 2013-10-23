struct CompactTestStructSmall {
  1: list<bool> bools,
  2: list<i32> ints,
  3: list<float> floats,
}

struct CompactTestStruct {
  1: i32 i1,
  2: i32 i2,
  3: i32 i3,
  4: bool b1,
  5: bool b2,
  6: list<double> doubles,
  7: set<i64> ints,
  8: map<string, i32> m1,
  9: map<i32, list<string>> m2,
  10: list<CompactTestStructSmall> structs,
  11: float f,
  12: map<i64, float> fmap,
  3681: string s,
}

exception CompactTestExn {
  1: CompactTestStruct t,
}

service CompactTestService {
  CompactTestStruct test(1: CompactTestStruct t) throws (1: CompactTestExn exn),
}
