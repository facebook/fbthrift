namespace cpp2 py3.simple

enum AnEnum {
  ONE = 1,
  TWO = 2,
  THREE = 3,
  FOUR = 4
}

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

struct ComplexStruct {
  1: SimpleStruct structOne
  2: SimpleStruct structTwo
  3: i32 an_integer
  4: string name
  5: AnEnum an_enum
}

const bool A_BOOL = true
const byte A_BYTE = 8
const i16 THE_ANSWER = 42
const i32 A_NUMBER = 84
const i64 A_BIG_NUMBER = 102
const double A_REAL_NUMBER = 3.14
const double A_FAKE_NUMBER = 3
const string A_WORD = "Good word"

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
  i32 sum_i16_list(1: list<i16> numbers)
  i32 sum_i32_list(1: list<i32> numbers)
  i32 sum_i64_list(1: list<i64> numbers)
  string concat_many(1: list<string> words)
  i32 count_structs(1: list<SimpleStruct> items)
  i32 sum_set(1: set<i32> numbers)
  bool contains_word(1: set<string> words, 2: string word)
  string get_map_value(1: map<string,string> words, 2: string key)
  i16 map_length(1: map<string,SimpleStruct> items)
  i16 sum_map_values(1: map<string,i16> items)
  i32 complex_sum_i32(1: ComplexStruct counter)
  string repeat_name(1: ComplexStruct counter)
  SimpleStruct get_struct()
  list<i32> fib(1: i16 n)
  set<string> unique_words(1: list<string> words)
  map<string,i16> words_count(1: list<string> words)
  AnEnum set_enum(1: AnEnum in_enum)
}
