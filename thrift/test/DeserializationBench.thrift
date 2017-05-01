cpp_include "folly/sorted_vector_types.h"

struct StructA {
  1: bool fieldA
  2: i32 fieldB
  3: string fieldC
  4: list<i32> fieldD
  5: set<i32> fieldE
  6: map<i32, string> fieldF
  7: list<list<list<i32>>> fieldG
  8: set<set<set<i32>>> fieldH
  9: map<map<i32, string>, map<i32, string>> fieldI
  10: map<list<set<i32>>, set<list<i32>>> fieldJ
}

typedef map<i32, string> (cpp.template = "folly::sorted_vector_map") folly_map
typedef set<i32> (cpp.template = "folly::sorted_vector_set") folly_set
typedef set<folly_set> (cpp.template = "folly::sorted_vector_set") folly_set_set
typedef map<folly_map, folly_map> (cpp.template = "folly::sorted_vector_map")
  folly_map_map
typedef set<list<i32>> (cpp.template = "folly::sorted_vector_set")
  folly_list_set
typedef map<list<folly_set>, folly_list_set>
  (cpp.template = "folly::sorted_vector_map") folly_list_set_map

struct StructB {
  1: bool fieldA
  2: i32 fieldB
  3: string fieldC
  4: list<i32> fieldD
  5: folly_set fieldE
  6: folly_map fieldF
  7: list<list<list<i32>>> fieldG
  8: set<folly_set_set> (cpp.template = "folly::sorted_vector_set") fieldH
  9: folly_map_map fieldI
  10: folly_list_set_map fieldJ
}
