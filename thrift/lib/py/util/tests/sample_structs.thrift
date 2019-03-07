namespace py thrift.test.sample_structs

struct DummyStruct {
  1: i32 a,
}

struct Struct {
   1: i32 a,
   2: list<i32> b,
   3: set<i32> c,
   4: map<i32, i32> d,
   5: map<i32, list<i32>> e,
   6: map<i32, set<i32>> f,
   7: list<DummyStruct> g,
   8: list<list<i32>> h,
   9: list<set<i32>> i,
 }
