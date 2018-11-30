namespace hack test.fixtures.jsenum

enum MyThriftEnum {
  foo = 1;
  bar = 2;
  baz = 3;
} (hack.attributes="ApiEnum, JSEnum")

struct MyThriftStruct {
  1: string foo (hack.attributes="FieldAttribute"),
  2: string bar (hack.visibility="private" hack.getter
    hack.getter_attributes="FieldGetterAttribute"),
  3: string baz (hack.getter hack.getter_attributes="FieldGetterAttribute"),
} (hack.attributes="ClassAttribute")

struct MySecondThriftStruct {
  1: MyThriftEnum foo (hack.visibility="private"),
  2: MyThriftStruct bar (hack.visibility="protected" hack.getter
    hack.getter_attributes="FieldStructGetterAttribute"),
  3: i64 baz (hack.getter hack.getter_attributes="FieldGetterAttribute"),
}

union UnionTesting {
  1: string foo (hack.visibility="protected" hack.getter
    hack.getter_attributes="FieldUnionGetterAttribute"),
  3: i64 bar (hack.getter hack.getter_attributes="FieldGetterAttribute"),
} (hack.union_enum_attributes="EnumAttributes")
