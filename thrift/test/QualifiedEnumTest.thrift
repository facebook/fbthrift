enum MyEnum { FOO, BAR }
const MyEnum FOO = BAR

struct MyStruct {
  1: optional MyEnum field1 = FOO;
  2: optional MyEnum field2 = MyEnum.FOO;
}
