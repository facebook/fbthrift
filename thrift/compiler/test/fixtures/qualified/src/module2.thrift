include "module0.thrift"
include "module1.thrift"

struct Struct {
  1: module0.Struct first,
  2: module1.Struct second,
}

struct BigStruct {
  1: Struct s,
  2: i32 id,
}
