namespace cpp2 test.frozen2

typedef map<i32, string> (cpp.template = 'std::unordered_map') HashMapI32String

struct StructA {
  1: i32 i32Field,
  2: string strField,
  5: list<HashMapI32String> listField
}
