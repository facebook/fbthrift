namespace java thrift.test

include "thrift/test/JsonToThriftTest/mySimpleStruct.thrift"

typedef map<string, mySimpleStruct.mySimpleStruct> simpleMap

struct myNestedMapStruct {
  1: map<string, simpleMap> maps
}
