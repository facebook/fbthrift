namespace java.swift test.fixtures.complex_union

typedef map<i16, string> containerTypedef

union ComplexUnion {
  1: i64 intValue;
  5: string stringValue;
  2: list<i64> intListValue;
  3: list<string> stringListValue;
  9: containerTypedef typedefValue;
  14: string stringRef (cpp2.ref = "true");
}

union VirtualComplexUnion {
  1: string thingOne;
  2: string thingTwo;
} (cpp.virtual)
