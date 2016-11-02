enum MyEnum {
  kMyFoo = 1,
  kMyBar = 2,
}

const MyEnum kFoo = MyEnum.kMyFoo

const MyEnum kBaz = 3

struct MyStruct {
  1: MyEnum baz,
}
