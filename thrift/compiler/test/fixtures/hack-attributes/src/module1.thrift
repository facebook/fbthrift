namespace hack test.fixtures.jsenum

enum MyThriftEnum {
  foo = 1;
  bar = 2;
  baz = 3;
} (hack.attributes="ApiEnum, JSEnum")

struct MyThriftStruct {
  1: string (hack.attributes="FieldAttribute") foo,
  2: string (hack.visibility="private"
    hack.getter hack.getter_attributes="FieldAttribute") bar,
  3: string (hack.getter hack.getter_attributes="FieldGetterAttribute") baz ,
} (hack.attributes="ClassAttribute")

struct MySecondThriftStruct {
  1: i64 (hack.visibility="private") foo,
  2: i64 (hack.visibility="protected") bar,
  3: i64 (hack.getter hack.getter_attributes="FieldGetterAttribute") baz,
}
