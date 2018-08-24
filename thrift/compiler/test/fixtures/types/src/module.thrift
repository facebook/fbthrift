include "include.thrift"

namespace cpp apache.thrift.fixtures.types
namespace cpp2 apache.thrift.fixtures.types

typedef binary TBinary

struct decorated_struct {
  1: string field,
} (cpp.declare_hash, cpp.declare_equal_to)

struct ContainerStruct {
  12: list<i32> fieldA
  2: list<i32> (cpp.template = "std::list") fieldB
  3: list<i32> (cpp.template = "std::deque") fieldC
  4: list<i32> (cpp.template = "folly::fbvector") fieldD
  5: list<i32> (cpp.template = "folly::small_vector") fieldE
  6: set<i32> (cpp.template = "folly::sorted_vector_set") fieldF
  7: map<i32, string> (cpp.template = "folly::sorted_vector_map") fieldG
  8: include.SomeMap fieldH
}

struct CppTypeStruct {
  1: list<i32> (cpp.type = "std::list<int32_t>") fieldA
}

enum has_bitwise_ops {
  none = 0,
  zero = 1,
  one = 2,
  two = 4,
  three = 8,
} (cpp.declare_bitwise_ops)

enum is_unscoped {
  hello = 0,
  world = 1,
} (cpp.deprecated_enum_unscoped)

service SomeService {
  include.SomeMap bounce_map(1: include.SomeMap m),
  map<TBinary, i64> binary_keyed_map(1: list<i64> r)
}

struct VirtualStruct {
  1: i64 MyIntField,
} (cpp.virtual)

struct MyStructWithForwardRefEnum {
  1: MyForwardRefEnum a = NONZERO,
  2: MyForwardRefEnum b = MyForwardRefEnum.NONZERO,
}

enum MyForwardRefEnum {
  ZERO = 0,
  NONZERO = 12,
}

struct TrivialNumeric {
  1: i32 a,
  2: bool b,
}

struct TrivialNestedWithDefault {
  1: i32 z = 4,
  2: TrivialNumeric n = {'a': 3, 'b': true},
}

struct ComplexString {
  1: string a,
  2: map<string, i32> b,
}

struct ComplexNestedWithDefault {
  1: string z = '4',
  2: ComplexString n = {'a': '3', 'b': {'a': 3}},
}

struct MinPadding {
  1: required byte small,
  2: required i64 big,
  3: required i16 medium,
  4: required i32 biggish,
  5: required byte tiny,
} (cpp.minimize_padding)
