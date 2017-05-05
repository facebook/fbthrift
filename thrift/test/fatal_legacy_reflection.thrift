namespace cpp apache.thrift.test

enum SampleEnum {
  kSampleEnumFoo = 0,
  kSampleEnumBar = 1,
}

struct SampleSubStruct {
  1: string string_sub_field,
}

union SampleSubUnion {
  1: string string_sub_field,
}

struct SampleStruct {
  1: bool bool_field,
  2: byte byte_field,
  3: i16 i16_field,
  4: i32 i32_field,
  5: i64 i64_field,
  6: double double_field,
  7: float float_field,
  8: binary binary_field,
  9: string string_field,
  10: SampleEnum enum_field,
  11: SampleSubStruct struct_field,
  12: SampleSubUnion union_field,
  13: list<i16> list_i16_field,
  14: set<i32> set_i32_field,
  15: map<i64, string> map_i64_string_field,
  16: list<bool> list_bool_field,
  17: list<byte> list_byte_field,
  18: list<double> list_double_field,
  19: list<float> list_float_field,
  20: list<binary> list_binary_field,
  21: string annotated_string_field (ann_key = 'ann_value')
}
