enum MyEnum {
  MyValue1 = 0,
  MyValue2 = 1,
}

struct MyStruct {
  1: i64 MyIntField,
  2: string MyStringField,
}

service MyService {
  bool hasDataById(1: i64 id)
  string getDataById(1: i64 id)
  void putDataById(1: i64 id, 2: string data)
  oneway void lobDataById(1: i64 id, 2: string data)
}

service MyServiceFast {
  bool hasDataById(1: i64 id) (thread='eb')
  string getDataById(1: i64 id) (thread='eb')
  void putDataById(1: i64 id, 2: string data) (thread='eb')
  oneway void lobDataById(1: i64 id, 2: string data) (thread='eb')
}
