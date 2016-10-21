namespace java.swift test.fixtures.basic

enum MyEnum {
  MyValue1,
  MyValue2,
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
