# @generated
# To regenerate `fbcode/thrift/test/gen_if`, invoke
#   `buck run //thrift/test:generate_thrift_files`


namespace cpp2 apache.thrift.test

struct struct_optional_map_string_string_cpp_ref {
  1: optional map<string, string> (cpp.ref = 'true') field_1;
  2: optional map<string, string> (cpp.ref = 'true') field_2;
}
