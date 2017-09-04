include "transitive.thrift"

namespace android_lite one.two.three
namespace java.swift test.fixtures.includes.includes

struct Included {
  1: i64 MyIntField = 0;
  2: transitive.Foo MyTransitiveField = transitive.ExampleFoo;
}

const Included ExampleIncluded = {
  MyIntField: 2,
  MyTransitiveField: transitive.ExampleFoo,
}

typedef i64 IncludedInt64

const i64 IncludedConstant = 42
