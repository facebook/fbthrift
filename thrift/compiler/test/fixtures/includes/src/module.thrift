include "includes.thrift"
namespace java.swift test.fixtures.includes

struct MyStruct {
  1: includes.Included MyIncludedField = { "MyIntField": 5 };
}
