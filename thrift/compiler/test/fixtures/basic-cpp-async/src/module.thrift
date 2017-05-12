enum MyEnum {
  MyValue1 = 0,
  MyValue2 = 1,
}

struct MyStruct {
  1: i64 MyIntField,
  2: string MyStringField,
}

service MyService {
  void ping()
  string getRandomData()
  bool hasDataById(1: i64 id)
  string getDataById(1: i64 id)
  void putDataById(1: i64 id, 2: string data)
  oneway void lobDataById(1: i64 id, 2: string data)
  void putStructById(1: i64 id, 2: MyStruct data)
}
