namespace cpp2 test.frozen2

struct TestStruct {
  1: i32 i32Field,
}

enum TestEnum {
  Foo = 1,
  Bar = 2,
}

enum RetType {
  THAWED = 0,
  FROZEN = 1,
}

struct ReturnPrimitives {
  1: i64 i64Arg,
  2: double doubleArg,
  3: bool boolArg,
  4: RetType type,
}

struct ReturnString {
  1: string strArg,
  2: RetType type,
}

struct ReturnContainers {
  1: list<i32> listArg,
  2: set<i32> stArg,
  3: map<i32, string> mapArg,
  4: RetType type,
}

struct ReturnStructs {
  1: TestStruct structArg,
  2: TestEnum enumArg,
  3: RetType type,
}

struct ReturnVoid {
  1: RetType type,
}

service InterfaceService {
  ReturnPrimitives testPrimitives(
      1: i64 i64Arg,
      2: double doubleArg,
      3: bool boolArg)
  ReturnString testString(1: string strArg)
  ReturnContainers testContainers(
      1: list<i32> listArg,
      2: set<i32> stArg,
      3: map<i32, string> mapArg)
  ReturnStructs testStructs(1: TestStruct structArg, 2: TestEnum enumArg)

  ReturnVoid testVoid()
}
