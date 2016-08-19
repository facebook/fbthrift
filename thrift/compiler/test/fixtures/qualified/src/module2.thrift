namespace cpp MODULE2
namespace cpp2 module2
namespace java module2
namespace py module2
namespace java.swift test.fixtures.module2

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

const Struct c2 = {
  "first": module0.c0,
  "second": module1.c1,
};
