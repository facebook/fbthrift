include "thrift/annotation/thrift.thrift"
package "meta.com/thrift/core/protocol/_tests_/compact_test"

namespace hack ""

struct CompactTestStructSmall {
  1: list<bool> bools;
  2: list<i32> ints;
  3: list<float> floats;
}

struct CompactTestStruct {
  1: i32 i1;
  2: i32 i2;
  3: i32 i3;
  4: bool b1;
  5: bool b2;
  6: list<double> doubles;
  7: set<i64> ints;
  8: map<string, i32> m1;
  9: map<i32, list<string>> m2;
  10: list<CompactTestStructSmall> structs;
  11: float f;
  12: map<i64, float> fmap;
  13: i16 i16_test;
  14: i32 i32_test;
  15: byte byte_test;
  3681: string s;
}

union SerializerTestUnion {
  1: i32 int_value;
  2: string str_value;
  3: list<string> list_of_strings;
  4: set<string> set_of_strings;
  5: map<i32, string> map_of_int_to_strings;
  6: CompactTestStructSmall test_struct;
}

/**
 * Subset of CompactTestStruct for TCompactProtocolAcceleratedTest
 */
struct CompactTestStructWithFieldsMissing {
  1: i32 i1;
  9: map<i32, list<string>> m2;
  3681: string s;
}

exception CompactTestExn {
  1: CompactTestStruct t;
}

service CompactTestService {
  CompactTestStruct test(1: CompactTestStruct t) throws (1: CompactTestExn exn);
}

@thrift.SerializeInFieldIdOrder
struct OrderedInSerialization {
  3: i32 c;
  1: i32 a;
  2: i32 b;
}

struct Ordered {
  1: i32 a;
  2: i32 b;
  3: i32 c;
}

struct Unordered {
  3: i32 c;
  1: i32 a;
  2: i32 b;
}
