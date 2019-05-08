namespace cpp2 apache.thrift.test

struct Struct1 {
   1: optional string str;
}

struct Struct2 {
  1: optional list<Struct1> structs;
}

struct MixedStruct {
  1: optional bool myBool,
  2: optional byte myByte,
  3: optional i16 myI16;
  4: optional i32 myI32;
  5: optional i64 myI64;
  6: optional double myDouble;
  7: optional float myFloat;
  8: optional string myString;
  9: optional binary myBinary;
  10: optional map<i32, string> myMap;
  11: optional list<string> myList;
  12: optional set<i64> mySet;
  13: optional Struct1 struct1;
  14: optional Struct2 struct2;
}
