namespace cpp2 apache.thrift.test
namespace java thrift.test

enum ENUM {
  ENUM_A = 0,
  ENUM_B = 1,
  ENUM_C = 2,
  ENUM_D = 3,
}

struct SmallStruct {
  1: list<bool> bools,
  2: list<i32> ints,
}

struct TypeTester {
  1: string s,
  2: bool b,
  3: byte y,
  4: i16 i,
  5: i32 j,
  6: i64 k,
  7: double d,
  8: ENUM e,
  9: list<i32> li,
  10: set<i32> si,
  11: map<i32, i32> mii,
}

struct NestedStruct {
  1: TypeTester t,
  2: list<list<list<i32>>> lists,
  3: map<string, map<i32, list<SmallStruct>>> maps,
  4: set<set<set<i32>>> sets,
}

struct TestStruct {
  1: i32 i1,
  2: i32 i2,
  3: i32 i3,
  4: bool b1,
  5: bool b2,
  6: list<double> doubles,
  7: set<i64> ints,
  8: map<string, i32> m1,
  9: map<i32, list<string>> m2,
  10: list<SmallStruct> structs,
  11: NestedStruct n,
  3681: string s,
}

exception TestException {
  1: TestStruct t,
}

service TestService {
  TestStruct test(1: TestStruct t)
      throws (1: TestException exn),
}
