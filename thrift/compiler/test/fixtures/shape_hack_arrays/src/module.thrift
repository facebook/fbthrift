struct A {
  1: string a;
  2: map<string, string> map_of_string_to_string;
}

struct B {
  1: map<string, string> map_of_string_to_string;
  2: map<string, i32> map_of_string_to_i32;
  3: map<string, A> map_of_string_to_A;
  4: map<string, B> map_of_string_to_self;
  5: map<string, list<A>> map_of_string_to_list_of_A;
  6: map<string, map<string, i32>> map_of_string_to_map_of_string_to_i32;
  7: map<string, map<string, A>> map_of_string_to_map_of_string_to_A;

  8: list<string> list_of_string;
  9: list<map<string, A>> list_of_map_of_string_to_A;
  10: list<B> list_of_self;
  11: list<list<B>> list_of_list_of_self;
  12: list<map<string, list<A>>> list_of_map_of_string_to_list_of_A;
}
