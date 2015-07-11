include "includes.thrift"

struct MyStruct {
  1: includes.Included MyIncludedField = { "MyIntField": 5 };
}
