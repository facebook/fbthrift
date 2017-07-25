struct SimpleStruct {
  1: bool a_bool
  2: byte a_byte
  3: i16 a_i16
  4: i32 a_i32
  5: i64 a_i64
  6: float a_float
  7: double a_double
  8: string a_string
  9: binary a_binary
}

struct ContainerStruct {
  1: list<bool> bool_list
  2: list<byte> byte_list
  3: list<i16> i16_list
  4: list<i32> i32_list
  5: list<i64> i64_list
  6: list<float> float_list
  7: list<double> double_list
  8: list<string> string_list
  9: list<binary> binary_list

  10: set<bool> boolean_set
  11: set<byte> byte_set
  12: set<i16> i16_set
  13: set<i32> i32_set
  14: set<i64> i64_set
  15: set<float> float_set
  16: set<double> double_set
  17: set<string> string_set
  18: set<binary> binary_set

  20: map<byte, bool> byte_bool_map
  21: map<i32, i16> i32_i16_map
  22: map<string, i64> string_i64_map
  23: map<double, binary> double_binary_map

  30: list<map<byte, bool>> list_byte_bool_map
  31: set<map<i32, i16>> i32_i16_map_set
  32: map<string, map<double, binary>> string_double_binary_map_map
}

struct ComplexStruct {
  1: SimpleStruct a_simple_struct
  2: ContainerStruct a_container_struct

  10: list<SimpleStruct> simple_struct_list
  11: list<ContainerStruct> container_struct_list
  12: set<SimpleStruct> simple_struct_set
  13: set<ContainerStruct> container_struct_set
  14: map<string, SimpleStruct> string_simple_struct_map
  15: map<string, ContainerStruct> string_container_struct_map
}
