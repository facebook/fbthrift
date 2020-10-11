# @generated
# To regenerate `fbcode/thrift/test/gen_if`, invoke
#   `buck run //thrift/test:generate_thrift_files`


namespace cpp2 apache.thrift.test

struct struct_required_string_cpp_ref {
  1: required string (cpp.ref = 'true') field_1;
  2: required string (cpp.ref = 'true') field_2;
}
