namespace cpp CppNamespaceOverride

enum E {
  Constant1 = 5,
  Constant2 = 120,
}

exception SampleException {
  1: string message
} (message = 'message')

struct X {
  1: string a,
  2: optional string b (field_annotation = 'something else'),
  3: map<string, X> c,
  5: list<i64> e,
  6: required bool f,
  4: set<i32> d,
} (type_annotation = 'sample')

service SampleService {
  list<i32> doSomething(1: map<string, X> foo) throws (1: SampleException ex)
}
