namespace cpp2 thrift.test

cpp_include "thrift/test/CustomStruct.h"

struct MyStruct {
  1: string stringData
  2: i32 intData
}

union MyUnion {
  1: string stringData
  2: i32 intData
}

typedef MyStruct (cpp.type = "MyCustomStruct") SpecializedStruct
typedef MyUnion (cpp.type = "MyCustomUnion") SpecializedUnion

struct Container {
  1: SpecializedStruct myStruct
  2: SpecializedUnion myUnion1
  3: SpecializedUnion myUnion2
  4: list<SpecializedStruct> myStructList
  5: list<SpecializedUnion> myUnionList
  6: map<i32, SpecializedStruct> myStructMap
  7: map<i32, SpecializedUnion> myUnionMap
  8: map<SpecializedStruct, string> myRevStructMap
  9: map<SpecializedUnion, string> myRevUnionMap
}

service CustomStruct {
  SpecializedStruct echoStruct(1: SpecializedStruct s);
  SpecializedUnion echoUnion(1: SpecializedUnion u);
}
