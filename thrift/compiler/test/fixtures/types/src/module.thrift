namespace cpp apache.thrift.fixtures.types
namespace cpp2 apache.thrift.fixtures.types

typedef map<string, i64>
  (cpp.declare_hash, cpp.declare_equal_to)
  decorated_map

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
}
