include "includes.thrift"
namespace java.swift test.fixtures.includes

struct MyStruct {
  1: includes.Included MyIncludedField = includes.ExampleIncluded;
  2: includes.Included MyOtherIncludedField;
  3: includes.IncludedInt64 MyIncludedInt = includes.IncludedConstant;
}
