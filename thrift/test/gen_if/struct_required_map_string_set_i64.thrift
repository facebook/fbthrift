# @generated
# To regenerate `fbcode/thrift/test/gen_if`, invoke
#   `buck run //thrift/test:generate_thrift_files`


namespace cpp2 apache.thrift.test

struct struct_required_map_string_set_i64 {
  1: required map<string, set<i64>> field_1;
  2: required map<string, set<i64>> field_2;
}
