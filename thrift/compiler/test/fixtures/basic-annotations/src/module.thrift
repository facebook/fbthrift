namespace java test.fixtures.basicannotations
namespace java.swift test.fixtures.basicannotations

enum MyEnum {
  MyValue1 = 0,
  MyValue2 = 1,
  DOMAIN = 2 (cpp.name = "REALM"),
}

struct MyStruct {
  # glibc has macros with this name, Thrift should be able to prevent collisions
  1: i64 major (cpp.name = "majorVer"),
  # package is a reserved keyword in Java, Thrift should be able to handle this
  2: string package (java.swift.name = "_package"),
  # should generate valid code even with double quotes in an annotation
  3: string annotation_with_quote (go.tag = 'tag:"somevalue"')
}

service MyService {
  void ping()
  string getRandomData()
  bool hasDataById(1: i64 id)
  string getDataById(1: i64 id)
  void putDataById(1: i64 id, 2: string data)
  oneway void lobDataById(1: i64 id, 2: string data)
  void doNothing() (cpp.name = 'cppDoNothing')
}

service MyServicePrioParent {
  void ping() (priority = 'IMPORTANT')
  void pong() (priority = 'HIGH_IMPORTANT')
} (priority = 'HIGH')

service MyServicePrioChild extends MyServicePrioParent {
  void pang() (priority = 'BEST_EFFORT')
}
