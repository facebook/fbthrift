enum MyEnum {
  MyValue1 = 0,
  MyValue2 = 1,
}

exception MyException1 {
  1: string message,
  2: optional MyEnum code,
}

exception MyException2 {
  1: string message,
  2: required MyEnum code,
}

exception MyException3 {
  1: string message,
  2: MyEnum code,
}

exception MyException4 {
  1: string message,
  2: MyEnum code = MyEnum.MyValue2,
}

exception MyException5 {
  1: string message,
  2: optional i64 code,
}
