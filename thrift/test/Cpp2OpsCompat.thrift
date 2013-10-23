namespace cpp thrift.test.cpp2ops

const i32 num_test_value = 42
const string str_test_value = "hello"

struct Compat {
  1: i32 num,
  2: string str,
}
