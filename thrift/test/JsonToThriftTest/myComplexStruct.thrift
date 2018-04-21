namespace cpp2 apache.thrift.test
namespace java thrift.test

include "thrift/test/JsonToThriftTest/mySimpleStruct.thrift"

enum EnumTest {
  EnumOne = 1,
  EnumTwo = 3,
}

exception ExceptionTest {
  2: string message;
}

struct myComplexStruct {
  1: mySimpleStruct.mySimpleStruct a,
  2: list<i16> b,
  3: map<string, mySimpleStruct.mySimpleStruct> c,
  4: EnumTest e,
  5: ExceptionTest x
}
