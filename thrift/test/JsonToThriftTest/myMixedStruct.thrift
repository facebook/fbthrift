namespace java thrift.test

struct mySuperSimpleStruct {
  1: i16 a,
}

struct myMixedStruct {
  1: list<i16> a,
  2: list<mySuperSimpleStruct> b,
  3: map<string, i16> c,
  4: map<string, mySuperSimpleStruct> d,
  5: set<i16> e,
}
