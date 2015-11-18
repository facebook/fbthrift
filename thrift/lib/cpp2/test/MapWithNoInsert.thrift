namespace cpp2 apache.thrift.test

cpp_include "thrift/lib/cpp2/test/MapWithNoInsert.h"

typedef
  map<string, i64> (cpp.template = "apache::thrift::test::MapWithNoInsert")
  MapWithNoInsertT

struct MyObject {
  1: MapWithNoInsertT data,
}
