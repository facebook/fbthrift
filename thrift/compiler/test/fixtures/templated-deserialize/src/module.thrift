typedef i64 (cpp.type = "Foo", cpp.indirection=".value") IndirectionA
typedef i32 (cpp.type = "Baz", cpp.indirection=".__value()") IndirectionC
typedef double (cpp.type = "Bar", cpp.indirection=".value") IndirectionB

enum MyEnumA {
  fieldA = 1
  fieldB = 2
  fieldC = 4
}

struct SmallStruct {
  1: bool small_A
  2: i32 small_B
}

struct containerStruct {
  1: bool fieldA
  2: map<string, bool> fieldB
  3: set<i32> fieldC = [1, 2, 3, 4]
  4: string fieldD
  5: string fieldE = "somestring"
  6: list<list<list<i32>>> fieldF
  7: map<string, map<string, map<string, i32>>> fieldG
  8: list<set<i32>> fieldH
  9: bool fieldI = true
  10: map<string, list<i32>> fieldJ = {
       "subfieldA" : [1, 4, 8, 12],
       "subfieldB" : [2, 5, 9, 13],
     }
  11: list<list<list<list<i32>>>> fieldK
  12: set<set<set<bool>>> fieldL
  13: map<set<list<i32>>, map<list<set<string>>, string>> fieldM
  14: list<IndirectionA> fieldN
  15: list<IndirectionB> fieldO
  16: list<IndirectionC> fieldP
  17: MyEnumA fieldQ
  18: map<string, bool> fieldR (cpp.ref)
  19: SmallStruct fieldS (cpp.ref_type="unique")
  20: SmallStruct fieldT (cpp.ref_type="shared")
  21: SmallStruct fieldU (cpp.ref_type="shared_const")
  23: SmallStruct fieldX (cpp.ref)
}
