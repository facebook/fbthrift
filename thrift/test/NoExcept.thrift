namespace cpp2 apache.thrift.test

struct Simple {
  1: i32 a,
  2: optional i32 b,
}

struct SimpleWithString {
  1: i32 a,
  2: optional i32 b,
  3: string c,
}

struct List {
  1: list<i32> a,
}

struct Set {
  1: set<i32> a,
}

struct Map {
  1: map<i32, string> a,
}

struct Complex {
  1: Simple a,
  2: list<Simple> b,
}

struct ComplexWithStringAndMap {
  1: SimpleWithString a,
  2: list<Simple> b,
  3: list<string> c,
  4: map<string, Simple> d,
}
