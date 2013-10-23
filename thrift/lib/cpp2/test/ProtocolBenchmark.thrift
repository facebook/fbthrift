namespace cpp apache.thrift.test
namespace cpp2 apache.thrift.test

struct IntOnly {
  1: i32 x,
}

struct StringOnly {
  1: string x,
}

struct BenchmarkObject {
  1: list<IntOnly> intStructs,
  2: list<StringOnly> stringStructs,
  3: list<i32> ints,
  4: list<string> strings,
}

