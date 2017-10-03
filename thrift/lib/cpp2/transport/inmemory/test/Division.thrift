namespace cpp2 apache.thrift

exception DivideByZero {
}

service Division {
  i32 divide(1: i32 x, 2: i32 y) throws (1: DivideByZero e)
}
