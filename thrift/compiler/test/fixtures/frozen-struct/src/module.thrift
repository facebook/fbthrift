include "include1.thrift"
include "include2.thrift"

namespace cpp2 some.ns

struct ModuleA {
  1: i32 i32Field,
  2: string strField,
  3: list<i16> listField,
  4: map<string, i32> mapField,
  5: include1.IncludedA inclAField,
  6: include2.IncludedB inclBField
}

enum EnumB {
  EMPTY = 1
}

struct ModuleB {
  1: i32 i32Field,
  2: EnumB inclEnumB
}
