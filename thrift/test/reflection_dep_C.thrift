namespace cpp2 test_cpp2.cpp_reflection

include "thrift/test/reflection_dep_D.thrift"

struct dep_C_struct {
  1: reflection_dep_D.dep_D_struct d
  2: i32 i_c
}
