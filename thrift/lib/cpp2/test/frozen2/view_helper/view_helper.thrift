namespace cpp2 test.frozen2

enum TestEnum {
  Foo = 1,
  Bar = 2,
}

struct TestStruct {
  1: i32 i32Field,
  2: string strField,
  3: double doubleField,
  4: bool boolField,
  5: list<string> listField,
  6: map<i32, string> mapField,
  7: TestEnum enumField,
}
