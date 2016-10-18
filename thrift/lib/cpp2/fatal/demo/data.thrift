namespace cpp2 static_reflection.demo

enum some_enum {
  field0,
  field1,
  field2,
}

struct flat_struct {
  1: i32 the_int;
  2: some_enum the_enum;
}

union some_variant {
  1: i32 integer_data;
  2: double fp_data;
  3: string string_data;
  4: flat_struct struct_data;
}

struct nested_struct {
  1: i32 int_field;
  2: bool bool_field;
  3: double fp_field;
  4: string string_field;
  5: list<flat_struct> struct_list_field;
  6: map<string, bool> map_field;
  7: set<i32> set_field;
  8: some_variant variant_field;
}

const flat_struct example_1 = {
  "the_int": 42,
  "the_enum": some_enum.field2
};

const some_variant example_2 = {
  "struct_data": {
    "the_int": 56,
    "the_enum": some_enum.field0
  }
};

const nested_struct example_3 = {
  "int_field": 98,
  "bool_field": 1,
  "fp_field": 7.2,
  "string_field": "HELLO, WORLD",
  "struct_list_field": [
    {
      "the_int": 0,
      "the_enum": some_enum.field0
    },
    {
      "the_int": 1,
      "the_enum": some_enum.field1
    },
    {
      "the_int": 2,
      "the_enum": some_enum.field2
    }
  ],
  "map_field": {
    "works": 1,
    "hard": 0,
    "worth it": 1
  },
  "set_field": {
  },
  "variant_field": {
    "fp_data": 0.5
  }
};
