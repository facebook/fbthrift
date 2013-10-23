namespace java thrift.test

struct myEmptyStruct {
}

struct myNestedEmptyStruct {
  1: myEmptyStruct a,
  2: list<myEmptyStruct> b,
  3: i32 c,
}
