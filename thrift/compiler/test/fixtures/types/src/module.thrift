namespace cpp apache.thrift.fixtures.types
namespace cpp2 apache.thrift.fixtures.types

typedef map<string, i64>
  (cpp.declare_hash, cpp.declare_equal_to)
  decorated_map

struct decorated_struct {
  1: string field,
} (cpp.declare_hash, cpp.declare_equal_to)
