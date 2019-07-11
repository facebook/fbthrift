namespace java test.fixtures.basic
namespace java.swift test.fixtures.basic

enum MyEnum {
  MyValue1 = 0,
  MyValue2 = 1,
}

struct MyStruct {
  1: i64 MyIntField,
  2: string MyStringField,
  # use the type before it is defined. Thrift should be able to handle this
  3: MyDataItem MyDataField,
  4: MyEnum myEnum,
}

struct MyDataItem {}

union MyUnion {
  1: MyEnum myEnum,
  2: MyStruct myStruct,
  3: MyDataItem myDataItem,
}

service MyService {
  void ping()
  string getRandomData()
  bool hasDataById(1: i64 id)
  string getDataById(1: i64 id)
  void putDataById(1: i64 id, 2: string data)
  oneway void lobDataById(1: i64 id, 2: string data)
}
