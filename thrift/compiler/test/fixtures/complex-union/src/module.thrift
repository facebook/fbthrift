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

union ListUnion {
  2: list<i64> intListValue;
  3: list<string> stringListValue;
}

union DataUnion {
  1: binary binaryData,
  2: string stringData,
}

struct Val {
  1: string strVal;
  2: i32 intVal;
  9: containerTypedef typedefValue;
}

union ValUnion {
  1: Val v1,
  2: Val v2,
}

union VirtualComplexUnion {
  1: string thingOne;
  2: string thingTwo;
} (cpp.virtual)
