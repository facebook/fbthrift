namespace cpp2 py3.simple

exception SimpleException {
  1: i16 err_code
}

struct SimpleStruct {
  1: bool is_on
  2: byte tiny_int
  3: i16 small_int
  4: i32 nice_sized_int
  5: i64 big_int
  6: double real
}

service SimpleService {
  i32 get_five()
  i32 add_five(1: i32 num)
  void do_nothing()
  string concat(1: string first, 2: string second)
  i32 get_value(1: SimpleStruct simple_struct)
  bool negate(1: bool input)
  byte tiny(1: byte input)
  i16 small(1: i16 input)
  i64 big(1: i64 input)
  double two(1: double input)
  void expected_exception() throws (1: SimpleException se)
  i32 unexpected_exception()
}
