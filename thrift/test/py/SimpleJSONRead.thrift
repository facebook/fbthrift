struct SomeStruct {
  1: i32 anInteger,
  2: map<string, double> aMap,
}

struct Stuff {
  1: string aString,
  2: i16 aShort,
  3: i32 anInteger,
  4: i64 aLong,
  5: double aDouble,
  6: bool aBool,
  7: binary aBinary,
  8: SomeStruct aStruct,
  9: list<list<list<list<string>>>> aList,
  10: map<i32, map<string, list<i32>>> aMap,
  11: string anotherString,
  12: list<SomeStruct> aListOfStruct,
  13: list<set<string>> aListOfSet,
  14: list<double> aListOfDouble,
}
