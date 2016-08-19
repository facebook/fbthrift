include "thrift/test/module_without_fatal.thrift"

namespace cpp2 module_with_fatal

struct parent_struct1 {
  1: module_without_fatal.mwof_struct1 field1
  2: module_without_fatal.mwof_enum1 field2
  3: module_without_fatal.mwof_union1 field3
}
