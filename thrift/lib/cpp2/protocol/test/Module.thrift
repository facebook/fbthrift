namespace cpp2 apache.thrift.test

struct StructWithEmptyMap {
  1: map<string, i64> myMap,
}

struct SubStruct {
  3: i64 mySubI64 = 17,
  12: string mySubString = "foobar",
}

union SubUnion {
  209: string text,
}

const SubUnion kSubUnion = {
  "text": "glorious",
}

struct OneOfEach {
  1: bool myBool = 1,
  2: byte myByte = 17,
  3: i16 myI16 = 1017,
  4: i32 myI32 = 100017,
  5: i64 myI64 = 5000000017,
  6: double myDouble = 5.25,
  7: float myFloat = 5.25,
  8: map<string, i64> myMap = {
    "foo": 13,
    "bar": 17,
    "baz": 19,
  },
  9: list<string> myList = [
    "foo",
    "bar",
    "baz",
  ],
  10: set<string> mySet = [
    "foo",
    "bar",
    "baz",
  ],
  11: SubStruct myStruct,
  12: SubUnion myUnion = kSubUnion,
}
