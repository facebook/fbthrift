namespace java.swift test.fixtures.complex_union

union ComplexUnion {
  1: i64 intValue;
  5: string stringValue;
  2: list<i64> intListValue;
  3: list<string> stringListValue;
  14: string stringRef (cpp2.ref = "true");
}

union FinalComplexUnion {
  1: string thingOne;
  2: string thingTwo;
} (final)
