namespace java.swift test.fixtures.complex_union

union ComplexUnion {
  1: i64 intValue;
  2: string stringValue;
  3: list<i64> intListValue;
  4: list<string> stringListValue;
  5: string stringRef (cpp2.ref = "true");
}
