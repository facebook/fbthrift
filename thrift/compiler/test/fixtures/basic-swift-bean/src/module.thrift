namespace java.swift test.fixtures.basic_swift_bean

enum MyEnum {
  MyValue1 = 0,
  MyValue2 = 1,
  DOMAIN = 2,
}

struct MyStruct1 {
  1: i64 MyIntField,
  2: string MyStringField,
  # use the type before it is defined. Thrift should be able to handle this
  3: MyDataItem MyDataField,
  # glibc has macros with this name, Thrift should be able to prevent collisions
  4: i64 major (cpp.name = "majorVer"),
}

struct MyStruct2 {
  1: MyStruct1 myStruct1,
  2: string myString,
}

struct MyDataItem {
  1: i32 field1,
  2: i32 field2,
}

union MyUnion1 {
  1: i32 i
  2: double d
}

union MyUnion2 {
  1: MyUnion1 u,
  2: MyStruct1 s,
}

const MyStruct1 ms = {
  "MyIntField": 42,
  "MyStringField": "Meaning_of_life",
  "MyDataField": {
    "field1": 1,
    "field2": 2,
  },
  "major": 32,
}
