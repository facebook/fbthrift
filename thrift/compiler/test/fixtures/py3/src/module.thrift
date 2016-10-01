namespace cpp2 py3.simple

struct SimpleStruct {
  1: i32 key
  2: i32 value
}

service SimpleService {
  i32 get_five()
  i32 add_five(1: i32 num)
  void do_nothing()
  string concat(1: string first, 2: string second)
  i32 get_value(1: SimpleStruct simple_struct)
}
