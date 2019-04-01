namespace java.swift test.fixtures.basic

enum MyEnum {
  MyValue1 = 0,
  MyValue2 = 1,
  DOMAIN = 2 (cpp.name = "REALM"),
}

struct MyStruct {
  1: i64 MyIntField,
  2: string MyStringField,
  # use the type before it is defined. Thrift should be able to handle this
  3: MyDataItem MyDataField,
  # glibc has macros with this name, Thrift should be able to prevent collisions
  4: i64 major (cpp.name = "majorVer"),
  5: MyEnum myEnum,
  # package is a reserved keyword in Java, Thrift should be able to handle this
  6: string package (java.swift.name = "_package"),
}

struct MyDataItem {}

service MyService {
  void ping()
  string getRandomData()
  bool hasDataById(1: i64 id)
  string getDataById(1: i64 id)
  void putDataById(1: i64 id, 2: string data)
  oneway void lobDataById(1: i64 id, 2: string data)
}

service MyServiceFast {
  void ping() (thread='eb')
  string getRandomData() (thread='eb')
  bool hasDataById(1: i64 id) (thread='eb')
  string getDataById(1: i64 id) (thread='eb')
  void putDataById(1: i64 id, 2: string data) (thread='eb')
  oneway void lobDataById(1: i64 id, 2: string data) (thread='eb')
}

service MyServiceEmpty {
}

service MyServicePrioParent {
  void ping() (priority = 'IMPORTANT')
  void pong() (priority = 'HIGH_IMPORTANT')
} (priority = 'HIGH')

service MyServicePrioChild extends MyServicePrioParent {
  void pang() (priority = 'BEST_EFFORT')
}
