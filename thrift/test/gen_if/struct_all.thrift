# @generated
# To regenerate `fbcode/thrift/test/gen_if`, invoke
#   `buck run //thrift/test:generate_thrift_files`

include "struct_i64.thrift"
include "struct_string.thrift"
include "struct_set_i64.thrift"
include "struct_set_string.thrift"
include "struct_map_string_i64.thrift"
include "struct_map_string_string.thrift"
include "struct_map_string_set_i64.thrift"
include "struct_map_string_set_string.thrift"
include "struct_optional_i64.thrift"
include "struct_optional_string.thrift"
include "struct_optional_set_i64.thrift"
include "struct_optional_set_string.thrift"
include "struct_optional_map_string_i64.thrift"
include "struct_optional_map_string_string.thrift"
include "struct_optional_map_string_set_i64.thrift"
include "struct_optional_map_string_set_string.thrift"
include "struct_required_i64.thrift"
include "struct_required_string.thrift"
include "struct_required_set_i64.thrift"
include "struct_required_set_string.thrift"
include "struct_required_map_string_i64.thrift"
include "struct_required_map_string_string.thrift"
include "struct_required_map_string_set_i64.thrift"
include "struct_required_map_string_set_string.thrift"
include "struct_i64_cpp_ref.thrift"
include "struct_string_cpp_ref.thrift"
include "struct_set_i64_cpp_ref.thrift"
include "struct_set_string_cpp_ref.thrift"
include "struct_map_string_i64_cpp_ref.thrift"
include "struct_map_string_string_cpp_ref.thrift"
include "struct_map_string_set_i64_cpp_ref.thrift"
include "struct_map_string_set_string_cpp_ref.thrift"
include "struct_optional_i64_cpp_ref.thrift"
include "struct_optional_string_cpp_ref.thrift"
include "struct_optional_set_i64_cpp_ref.thrift"
include "struct_optional_set_string_cpp_ref.thrift"
include "struct_optional_map_string_i64_cpp_ref.thrift"
include "struct_optional_map_string_string_cpp_ref.thrift"
include "struct_optional_map_string_set_i64_cpp_ref.thrift"
include "struct_optional_map_string_set_string_cpp_ref.thrift"
include "struct_required_i64_cpp_ref.thrift"
include "struct_required_string_cpp_ref.thrift"
include "struct_required_set_i64_cpp_ref.thrift"
include "struct_required_set_string_cpp_ref.thrift"
include "struct_required_map_string_i64_cpp_ref.thrift"
include "struct_required_map_string_string_cpp_ref.thrift"
include "struct_required_map_string_set_i64_cpp_ref.thrift"
include "struct_required_map_string_set_string_cpp_ref.thrift"

namespace cpp2 apache.thrift.test

struct struct_all {
  1: struct_i64.struct_i64 field_1;
  2: struct_string.struct_string field_2;
  3: struct_set_i64.struct_set_i64 field_3;
  4: struct_set_string.struct_set_string field_4;
  5: struct_map_string_i64.struct_map_string_i64 field_5;
  6: struct_map_string_string.struct_map_string_string field_6;
  7: struct_map_string_set_i64.struct_map_string_set_i64 field_7;
  8: struct_map_string_set_string.struct_map_string_set_string field_8;
  9: struct_optional_i64.struct_optional_i64 field_9;
  10: struct_optional_string.struct_optional_string field_10;
  11: struct_optional_set_i64.struct_optional_set_i64 field_11;
  12: struct_optional_set_string.struct_optional_set_string field_12;
  13: struct_optional_map_string_i64.struct_optional_map_string_i64 field_13;
  14: struct_optional_map_string_string.struct_optional_map_string_string field_14;
  15: struct_optional_map_string_set_i64.struct_optional_map_string_set_i64 field_15;
  16: struct_optional_map_string_set_string.struct_optional_map_string_set_string field_16;
  17: struct_required_i64.struct_required_i64 field_17;
  18: struct_required_string.struct_required_string field_18;
  19: struct_required_set_i64.struct_required_set_i64 field_19;
  20: struct_required_set_string.struct_required_set_string field_20;
  21: struct_required_map_string_i64.struct_required_map_string_i64 field_21;
  22: struct_required_map_string_string.struct_required_map_string_string field_22;
  23: struct_required_map_string_set_i64.struct_required_map_string_set_i64 field_23;
  24: struct_required_map_string_set_string.struct_required_map_string_set_string field_24;
  25: struct_i64_cpp_ref.struct_i64_cpp_ref field_25;
  26: struct_string_cpp_ref.struct_string_cpp_ref field_26;
  27: struct_set_i64_cpp_ref.struct_set_i64_cpp_ref field_27;
  28: struct_set_string_cpp_ref.struct_set_string_cpp_ref field_28;
  29: struct_map_string_i64_cpp_ref.struct_map_string_i64_cpp_ref field_29;
  30: struct_map_string_string_cpp_ref.struct_map_string_string_cpp_ref field_30;
  31: struct_map_string_set_i64_cpp_ref.struct_map_string_set_i64_cpp_ref field_31;
  32: struct_map_string_set_string_cpp_ref.struct_map_string_set_string_cpp_ref field_32;
  33: struct_optional_i64_cpp_ref.struct_optional_i64_cpp_ref field_33;
  34: struct_optional_string_cpp_ref.struct_optional_string_cpp_ref field_34;
  35: struct_optional_set_i64_cpp_ref.struct_optional_set_i64_cpp_ref field_35;
  36: struct_optional_set_string_cpp_ref.struct_optional_set_string_cpp_ref field_36;
  37: struct_optional_map_string_i64_cpp_ref.struct_optional_map_string_i64_cpp_ref field_37;
  38: struct_optional_map_string_string_cpp_ref.struct_optional_map_string_string_cpp_ref field_38;
  39: struct_optional_map_string_set_i64_cpp_ref.struct_optional_map_string_set_i64_cpp_ref field_39;
  40: struct_optional_map_string_set_string_cpp_ref.struct_optional_map_string_set_string_cpp_ref field_40;
  41: struct_required_i64_cpp_ref.struct_required_i64_cpp_ref field_41;
  42: struct_required_string_cpp_ref.struct_required_string_cpp_ref field_42;
  43: struct_required_set_i64_cpp_ref.struct_required_set_i64_cpp_ref field_43;
  44: struct_required_set_string_cpp_ref.struct_required_set_string_cpp_ref field_44;
  45: struct_required_map_string_i64_cpp_ref.struct_required_map_string_i64_cpp_ref field_45;
  46: struct_required_map_string_string_cpp_ref.struct_required_map_string_string_cpp_ref field_46;
  47: struct_required_map_string_set_i64_cpp_ref.struct_required_map_string_set_i64_cpp_ref field_47;
  48: struct_required_map_string_set_string_cpp_ref.struct_required_map_string_set_string_cpp_ref field_48;
}
