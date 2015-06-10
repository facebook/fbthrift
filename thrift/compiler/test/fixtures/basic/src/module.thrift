enum MyEnum {
  MyValue1,
  MyValue2,
}

struct MyStruct {
  1: i64 MyIntField,
  2: string MyStringField,
}

service MyService {
  string getDataById(1: i64 id)
}
