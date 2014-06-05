namespace java thrift.test

struct mySuperSimpleDefaultStruct {
  1: list<i16> a = [99],
}

struct myDefaultStruct {
  1: list<i16> a = [1, 2, 3],
  2: list<mySuperSimpleDefaultStruct> b = [{}],
  3: map<string, i16> c = {"foo": 1},
  4: map<string, mySuperSimpleDefaultStruct> d = {"bar": {}},
  5: set<i16> e = [42, 74],
}
