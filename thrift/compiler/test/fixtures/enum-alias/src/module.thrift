namespace java.swift test.fixtures.enum_alias

enum MyEnum {
  MyValue1 = 0,
  MyValue2 = 1,
  MyValue2_dup = 1,
} (thrift.duplicate_values)

const MyEnum confused = MyEnum.MyValue2_dup
