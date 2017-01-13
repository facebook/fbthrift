cpp_include "thrift/test/fatal_reflection_indirection_types.h"

namespace cpp2 reflection_indirection

typedef i32 (cpp.type = 'CppFakeI32') FakeI32
typedef i32 (cpp.type = 'CppHasANumber', cpp.indirection = '.number') HasANumber
typedef i32 (cpp.type = 'CppHasAResult', cpp.indirection = '.foo().result()')
    HasAResult

struct struct_with_indirections {
  1: i32 real,
  2: FakeI32 fake,
  3: HasANumber number,
  4: HasAResult result,
}
