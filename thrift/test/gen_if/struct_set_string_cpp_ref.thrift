# @generated
# To regenerate `fbcode/thrift/test/gen_if`, invoke
#   `buck run //thrift/test:generate_thrift_files`


namespace cpp2 apache.thrift.test

struct struct_set_string_cpp_ref {
  1: set<string> (cpp.ref = 'true') field_1;
  2: set<string> (cpp.ref = 'true') field_2;
}
