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
} (java.swift.mutable = "true")

struct MyStruct2 {
  1: MyStruct1 myStruct1,
  2: string myString,
} (java.swift.mutable = "true")

struct MyDataItem {
  1: i32 field1,
  2: i32 field2,
} (java.swift.mutable = "true")

struct LegacyStruct {
  1: i32 normal,
  -1: i32 bad,
}

union ListUnion {
  2: list<i64> intListValue;
  3: list<string> stringListValue;
} (java.swift.mutable = "true")

union DataUnion {
  1: binary binaryData,
  2: string stringData,
} (java.swift.mutable = "true")

union MyUnion1 {
  1: i32 i
  2: double d
} (java.swift.mutable = "true")

union MyUnion2 {
  1: MyUnion1 u,
  2: MyStruct1 s,
} (java.swift.mutable = "true")

exception MyException {
  1: i32 errorCode,
  2: string message,
} (java.swift.mutable = "true")

const MyStruct1 ms = {
  "MyIntField": 42,
  "MyStringField": "Meaning_of_life",
  "MyDataField": {
    "field1": 1,
    "field2": 2,
  },
  "major": 32,
}

service legacy_service {
  map<string, list<i32>> getPoints(1: set<string> key, -1: i64 legacyStuff);
}
