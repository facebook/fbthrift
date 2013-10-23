/*
 * Copyright 2013 Facebook
 * @author Dmytro Dzhulgakov (dzhulgakov@fb.com)
 */

cpp_include "<unordered_map>"
cpp_include "thrift/test/OpaqueTest.h"

typedef
  double (cpp.type = "OpaqueDouble1", cpp.indirection=".__value()")
  OpaqueDouble1
typedef
  double (cpp.type = "OpaqueDouble2", cpp.indirection=".__value()")
  OpaqueDouble2
typedef
  i64 (cpp.type = "NonConvertibleId", cpp.indirection=".__value()")
  NonConvertibleId
typedef
  map<i32, OpaqueDouble1> (cpp.template = "std::unordered_map")
  OpaqueValuedMap

struct OpaqueTestStruct {
  1: OpaqueDouble1 d1,
  2: OpaqueDouble2 d2,
  3: OpaqueValuedMap dmap,
  4: list<NonConvertibleId> ids,
}
