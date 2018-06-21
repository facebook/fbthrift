namespace hack test.fixtures.jsenum

enum MyThriftEnum {
  foo = 1;
  bar = 2;
  baz = 3;
} (hack.attributes="ApiEnum, JSEnum")

struct MyThriftStruct {
  1: string foo,
  2: string bar,
  3: string baz,
}

struct MySecondThriftStruct {
  1: i64 foo,
  2: i64 bar,
  3: i64 baz,
}
