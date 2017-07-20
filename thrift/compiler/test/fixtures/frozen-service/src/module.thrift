namespace cpp2 some.ns

struct ModuleA {
  1: i32 i32Field,
  2: string strField,
  3: list<i16> listField,
  4: map<string, i32> mapField,
}

struct ModuleB {
  1: i32 i32Field,
}

exception ExceptionA {
  1: i32 code,
  2: string msg,
}

exception ExceptionB {
  1: i32 code,
  2: string msg,
}

service ServiceA {
  void moduleAMethod(1: ModuleA modArg),
  void moduleBMethod(1: ModuleB modArg),
  void i32StrDoubleMethod(1: i32 i32Arg, 2: string strArg, 3: double doubleArg),
  void versioningMethod(1: i32 i32Arg, 5: string strArg, 7: double doubleArg),

  i32 retI32Method(),
  ModuleA retModAMethod(),

  void throwMethod() throws (1: ExceptionA ea),
  void multiThrowMethod() throws (1: ExceptionA ea, 2: ExceptionB eb),
  void i32ThrowMethod(1: i32 i32Arg) throws (1: ExceptionA ea),
  void moduleAThrowMethod(1: ModuleA modArg) throws (1: ExceptionA ea),
  string mixedMethod(1: string strArg, 3: i32 i32Arg, 6: ModuleB modArg) throws (1: ExceptionA ea, 2: ExceptionB eb),
}
