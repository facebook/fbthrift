namespace cpp test_cpp1.cpp_frozen1
namespace cpp2 test_cpp2.cpp_frozen1
namespace d test_d.cpp_frozen1
namespace java test_java.cpp_frozen1
namespace java.swift test_swift.cpp_frozen1
namespace php test_php.cpp_frozen1
namespace python test_py.cpp_froze1

enum enum1 {
  e1_field0 = 110,
  e1_field1 = 115,
  e1_field2 = 120
}

struct struct_bool {
  153: bool data
}

const struct_bool struct_bool_f_c = {
  "data": false
};

const struct_bool struct_bool_t_c = {
  "data": true
};

struct struct_byte {
  153: byte data
}

const struct_byte struct_byte_c = {
  "data": 64
};

struct struct_e1 {
  153: enum1 data
}

const struct_e1 struct_e1_c = {
  "data": enum1.e1_field2
};

struct struct_i16 {
  153: i16 data
}

const struct_i16 struct_i16_c = {
  "data": 65
};

struct struct_i32 {
  153: i32 data
}

const struct_i32 struct_i32_c = {
  "data": 66
};

struct struct_i64 {
  153: i64 data
}

const struct_i64 struct_i64_c = {
  "data": 67
};

struct struct_flt {
  153: float data
}

const struct_flt struct_flt_c = {
  "data": 5.6
};

struct struct_dbl {
  153: double data
}

const struct_dbl struct_dbl_c = {
  "data": 7.2
};

struct struct_str {
  153: string data
}

const struct_str struct_str_c = {
  "data": "hello"
};

struct struct_lst_i32 {
  153: list<i32> data
}

const struct_lst_i32 struct_lst_i32_c = {
  "data": [12, 34, 56, 78]
};

struct struct_lst_str {
  153: list<string> data
}

const struct_lst_str struct_lst_str_c = {
  "data": ["foo", "bar", "baz"]
};

struct struct_req_lst_i32 {
  153: required list<i32> data
}

const struct_req_lst_i32 struct_req_lst_i32_c = {
  "data": [12, 34, 56, 78]
};

struct struct_req_lst_str {
  153: required list<string> data
}

const struct_req_lst_str struct_req_lst_str_c = {
  "data": ["foo", "bar", "baz"]
};

struct struct_set_i32 {
  153: set<i32> data
}

const struct_set_i32 struct_set_i32_c = {
  "data": [12, 34, 56, 78]
};

struct struct_set_str {
  153: set<string> data
}

const struct_set_str struct_set_str_c = {
  "data": ["foo", "bar", "baz"]
};

struct struct_req_set_i32 {
  153: required set<i32> data
}

const struct_req_set_i32 struct_req_set_i32_c = {
  "data": [12, 34, 56, 78]
};

struct struct_req_set_str {
  153: required set<string> data
}

const struct_req_set_str struct_req_set_str_c = {
  "data": ["foo", "bar", "baz"]
};

struct struct_map_i32_i32 {
  153: map<i32, i32> data
}

const struct_map_i32_i32 struct_map_i32_i32_c = {
  "data": {
    12: 34,
    56: 78
    9: 0
  }
};

struct struct_map_i32_str {
  153: map<i32, string> data
}

const struct_map_i32_str struct_map_i32_str_c = {
  "data": {
    12: "foo",
    34: "bar",
    56: "baz"
  }
};

struct struct_map_str_str {
  153: map<string, string> data
}

const struct_map_str_str struct_map_str_str_c = {
  "data": {
    "12": "34",
    "56": "78"
    "9": "0"
  }
};

struct struct_req_map_i32_i32 {
  153: required map<i32, i32> data
}

const struct_req_map_i32_i32 struct_req_map_i32_i32_c = {
  "data": {
    12: 34,
    56: 78
    9: 0
  }
};

struct struct_req_map_i32_str {
  153: required map<i32, string> data
}

const struct_req_map_i32_str struct_req_map_i32_str_c = {
  "data": {
    12: "foo",
    34: "bar",
    56: "baz"
  }
};

struct struct_req_map_str_str {
  153: required map<string, string> data
}

const struct_req_map_str_str struct_req_map_str_str_c = {
  "data": {
    "12": "34",
    "56": "78"
    "9": "0"
  }
};

struct struct_hmap_i32_i32 {
  153: hash_map<i32, i32> data
}

const struct_hmap_i32_i32 struct_hmap_i32_i32_c = {
  "data": {
    12: 34,
    56: 78
    9: 0
  }
};

struct struct_hmap_i32_str {
  153: hash_map<i32, string> data
}

const struct_hmap_i32_str struct_hmap_i32_str_c = {
  "data": {
    12: "foo",
    34: "bar",
    56: "baz"
  }
};

struct struct_hmap_str_str {
  153: hash_map<string, string> data
}

const struct_hmap_str_str struct_hmap_str_str_c = {
  "data": {
    "12": "34",
    "56": "78"
    "9": "0"
  }
};

struct struct_req_hmap_i32_i32 {
  153: required hash_map<i32, i32> data
}

const struct_req_hmap_i32_i32 struct_req_hmap_i32_i32_c = {
  "data": {
    12: 34,
    56: 78
    9: 0
  }
};

struct struct_req_hmap_i32_str {
  153: required hash_map<i32, string> data
}

const struct_req_hmap_i32_str struct_req_hmap_i32_str_c = {
  "data": {
    12: "foo",
    34: "bar",
    56: "baz"
  }
};

struct struct_req_hmap_str_str {
  153: required hash_map<string, string> data
}

const struct_req_hmap_str_str struct_req_hmap_str_str_c = {
  "data": {
    "12": "34",
    "56": "78"
    "9": "0"
  }
};

struct int_fields {
  1: required bool b1
  2: required bool b2
  3: required byte y3
  4: required bool b4
  5: required i16 s5
  6: required i32 i6
  7: required i16 s7
  8: required byte y8
  9: required i64 l9
  10: required byte y10
}

const int_fields int_fields_0_c = {
  "b1": 0,
  "b2": 0,
  "y3": 0x53,
  "b4": 0,
  "s5": 0x5555,
  "i6": 0x56565656,
  "s7": 0x5757,
  "y8": 0x58,
  "l9": 0x5959595959595959,
  "y10": 0x60,
};

const int_fields int_fields_1_c = {
  "b1": 1,
  "b2": 0,
  "y3": 0x53,
  "b4": 0,
  "s5": 0x5555,
  "i6": 0x56565656,
  "s7": 0x5757,
  "y8": 0x58,
  "l9": 0x5959595959595959,
  "y10": 0x60,
};

const int_fields int_fields_2_c = {
  "b1": 0,
  "b2": 1,
  "y3": 0x53,
  "b4": 0,
  "s5": 0x5555,
  "i6": 0x56565656,
  "s7": 0x5757,
  "y8": 0x58,
  "l9": 0x5959595959595959,
  "y10": 0x60,
};

const int_fields int_fields_3_c = {
  "b1": 1,
  "b2": 1,
  "y3": 0x53,
  "b4": 0,
  "s5": 0x5555,
  "i6": 0x56565656,
  "s7": 0x5757,
  "y8": 0x58,
  "l9": 0x5959595959595959,
  "y10": 0x60,
};

const int_fields int_fields_4_c = {
  "b1": 0,
  "b2": 0,
  "y3": 0x53,
  "b4": 1,
  "s5": 0x5555,
  "i6": 0x56565656,
  "s7": 0x5757,
  "y8": 0x58,
  "l9": 0x5959595959595959,
  "y10": 0x60,
};

const int_fields int_fields_5_c = {
  "b1": 1,
  "b2": 0,
  "y3": 0x53,
  "b4": 1,
  "s5": 0x5555,
  "i6": 0x56565656,
  "s7": 0x5757,
  "y8": 0x58,
  "l9": 0x5959595959595959,
  "y10": 0x60,
};

const int_fields int_fields_6_c = {
  "b1": 0,
  "b2": 1,
  "y3": 0x53,
  "b4": 1,
  "s5": 0x5555,
  "i6": 0x56565656,
  "s7": 0x5757,
  "y8": 0x58,
  "l9": 0x5959595959595959,
  "y10": 0x60,
};

const int_fields int_fields_7_c = {
  "b1": 1,
  "b2": 1,
  "y3": 0x53,
  "b4": 1,
  "s5": 0x5555,
  "i6": 0x56565656,
  "s7": 0x5757,
  "y8": 0x58,
  "l9": 0x5959595959595959,
  "y10": 0x60,
};

const int_fields int_fields_10_c = {
  "b1": 0,
  "b2": 0,
  "y3": 0x54,
  "b4": 0,
  "s5": 0x5555,
  "i6": 0x56565656,
  "s7": 0x5757,
  "y8": 0x58,
  "l9": 0x5959595959595959,
  "y10": 0x60,
};

const int_fields int_fields_11_c = {
  "b1": 1,
  "b2": 0,
  "y3": 0x54,
  "b4": 0,
  "s5": 0x5555,
  "i6": 0x56565656,
  "s7": 0x5757,
  "y8": 0x58,
  "l9": 0x5959595959595959,
  "y10": 0x60,
};

const int_fields int_fields_12_c = {
  "b1": 0,
  "b2": 1,
  "y3": 0x54,
  "b4": 0,
  "s5": 0x5555,
  "i6": 0x56565656,
  "s7": 0x5757,
  "y8": 0x58,
  "l9": 0x5959595959595959,
  "y10": 0x60,
};

const int_fields int_fields_13_c = {
  "b1": 1,
  "b2": 1,
  "y3": 0x54,
  "b4": 0,
  "s5": 0x5555,
  "i6": 0x56565656,
  "s7": 0x5757,
  "y8": 0x58,
  "l9": 0x5959595959595959,
  "y10": 0x60,
};

const int_fields int_fields_14_c = {
  "b1": 0,
  "b2": 0,
  "y3": 0x54,
  "b4": 1,
  "s5": 0x5555,
  "i6": 0x56565656,
  "s7": 0x5757,
  "y8": 0x58,
  "l9": 0x5959595959595959,
  "y10": 0x60,
};

const int_fields int_fields_15_c = {
  "b1": 1,
  "b2": 0,
  "y3": 0x54,
  "b4": 1,
  "s5": 0x5555,
  "i6": 0x56565656,
  "s7": 0x5757,
  "y8": 0x58,
  "l9": 0x5959595959595959,
  "y10": 0x60,
};

const int_fields int_fields_16_c = {
  "b1": 0,
  "b2": 1,
  "y3": 0x54,
  "b4": 1,
  "s5": 0x5555,
  "i6": 0x56565656,
  "s7": 0x5757,
  "y8": 0x58,
  "l9": 0x5959595959595959,
  "y10": 0x60,
};

const int_fields int_fields_17_c = {
  "b1": 1,
  "b2": 1,
  "y3": 0x54,
  "b4": 1,
  "s5": 0x5555,
  "i6": 0x56565656,
  "s7": 0x5757,
  "y8": 0x58,
  "l9": 0x5959595959595959,
  "y10": 0x60,
};

struct basic_fields {
  1: required bool b1
  2: required bool b2
  3: required byte y3
  4: required bool b4
  5: required i16 s5
  6: required i32 i6
  7: required i16 s7
  8: required byte y8
  9: required i64 l9
  10: required byte y10
  11: required string z11
}

const basic_fields basic_fields_c = {
  "b1": 1,
  "b2": 0,
  "y3": 0x53,
  "b4": 1,
  "s5": 0x5555,
  "i6": 0x56565656,
  "s7": 0x5757,
  "y8": 0x58,
  "l9": 0x5959595959595959,
  "y10": 0x60,
  "z11": "\x61\x61\x61\x61\x61",
};

struct multi_fields_sparse {
  1: bool b1
  2: bool b2
  3: byte y3
  4: bool b4
  5: i16 s5
  6: i32 i6
  7: i16 s7
  8: byte y8
  9: i64 l9
  10: byte y10
  11: string z11

  22: required bool b22
  23: required bool b23
  24: required byte y24
  25: required bool b25
  26: required i16 s26
  27: required i32 i27
  28: required i16 s28
  29: required byte y29
  30: required i64 l30
  31: required byte y31
  32: required string z32

  43: optional bool b43
  44: optional bool b44
  45: optional byte y45
  46: optional bool b46
  47: optional i16 s47
  48: optional i32 i48
  49: optional i16 s49
  50: optional byte y50
  51: optional i64 l51
  52: optional byte y52
  53: optional string z53
}

const multi_fields_sparse multi_fields_sparse_c = {
  "b1": 1,
  "b2": 0,
  "y3": 0x53,
  "b4": 1,
  "s5": 0x5555,
  "i6": 0x56565656,
  "s7": 0x5757,
  "y8": 0x58,
  "l9": 0x5959595959595959,
  "y10": 0x60,
  "z11": "\x61\x61\x61\x61\x61",
  "b22": 1,
  "b23": 0,
  "y24": 0x63,
  "b25": 1,
  "s26": 0x6565,
  "i27": 0x66666666,
  "s28": 0x6767,
  "y29": 0x68,
  "l30": 0x6969696969696969,
  "y31": 0x70,
  "z32": "\x71\x71\x71\x71\x71\x71\x71",
  "b43": 1,
  "b44": 0,
  "y45": 0x73,
  "b46": 1,
  "s47": 0x7575,
  "i48": 0x76767676,
  "s49": 0x7777,
  "y50": 0x78,
  "l51": 0x7979797979797979,
  "y52": 0x80,
  "z53": "\x81\x81",
};

struct padded_booleans {
  1: optional bool b1
  2: optional bool b2
  3: optional bool b3
  4: optional bool b4
  5: optional bool b5
  6: optional bool b6
  7: optional bool b7
  8: optional bool b8
  9: optional bool b9
  10: optional bool b10
  11: optional bool b11
  12: optional bool b12
  13: optional bool b13
  14: optional bool b14
  15: required bool b15
  16: required bool b16
  17: required bool b17
  18: required bool b18
  19: bool b19
  20: bool b20
  21: bool b21
  22: bool b22
  23: bool b23
  24: bool b24
  25: bool b25
  26: bool b26
}

const padded_booleans padded_booleans_0_c = {
  "b1": true,
  "b2": false,
  "b5": true,
  "b7": true,
  "b8": true,
  "b11": false,
  "b13": true,
  "b15": false,
  "b16": false,
  "b17": true,
  "b20": true,
  "b21": false,
}

struct unpadded_booleans {
  1: optional bool b1
  2: optional bool b2
  3: optional bool b3
  4: optional bool b4
  5: optional bool b5
  6: optional bool b6
  7: optional bool b7
  8: optional bool b8
  9: optional bool b9
  10: optional bool b10
  11: optional bool b11
  12: optional bool b12
  13: optional bool b13
  14: optional bool b14
  15: required bool b15
  16: required bool b16
  17: required bool b17
  18: required bool b18
  19: bool b19
  20: bool b20
  21: bool b21
  22: bool b22
  23: bool b23
  24: bool b24
}

const unpadded_booleans unpadded_booleans_0_c = {
  "b1": true,
  "b2": false,
  "b5": true,
  "b7": true,
  "b8": true,
  "b11": false,
  "b13": true,
  "b15": false,
  "b16": false,
  "b17": true,
  "b20": true,
  "b21": false,
}

struct nested {
  1: bool a
  2: byte b
  3: enum1 c
  4: struct_bool d
  5: bool e
  6: i64 f
  7: struct_e1 g
  8: struct_dbl h
}

const nested nested_c = {
  "a": false,
  "b": 0x91,
  "c": enum1.e1_field1,
  "d": {
    "data": true
  },
  "e": true,
  "f": 0x7272727272727272,
  "g": {
    "data": enum1.e1_field0
  },
  "h": {
    "data": 123.456
  }
}

struct variable_nested {
  1: string a
  2: bool b
  3: byte c
  4: enum1 d
  5: struct_bool e
  6: bool f
  7: i64 g
  8: struct_e1 h
  9: struct_dbl i
  10: string j
}

const variable_nested variable_nested_c = {
  "a": "hello",
  "b": false,
  "c": 0x91,
  "d": enum1.e1_field1,
  "e": {
    "data": true
  },
  "f": true,
  "g": 0x7272727272727272,
  "h": {
    "data": enum1.e1_field0
  },
  "i": {
    "data": 123.456
  },
  "j": "world"
}

struct variable_nested_lst {
  1: list<variable_nested> data
}

const variable_nested_lst variable_nested_lst_c = {
  "data": [
    {
      "a": "hello",
      "b": false,
      "c": 0x91,
      "d": enum1.e1_field1,
      "e": {
        "data": true
      },
      "f": true,
      "g": 0x7272727272727272,
      "h": {
        "data": enum1.e1_field0
      },
      "i": {
        "data": 123.456
      },
      "j": "world"
    },
    {
      "a": "foo",
      "b": true,
      "c": 0xaa,
      "d": enum1.e1_field0,
      "e": {
        "data": false
      },
      "f": false,
      "g": 0x12345678,
      "h": {
        "data": enum1.e1_field2
      },
      "i": {
        "data": 321.87
      },
      "j": "bar"
    },
    {
      "a": "some string",
      "b": true,
      "c": 0x18,
      "d": enum1.e1_field2,
      "e": {
        "data": true
      },
      "i": {
        "data": 123.456
      },
      "j": "another string"
    }
  ]
}

struct variable_nested_req_lst {
  1: required list<variable_nested> data
}

const variable_nested_req_lst variable_nested_req_lst_c = {
  "data": [
    {
      "a": "hello",
      "b": false,
      "c": 0x91,
      "d": enum1.e1_field1,
      "e": {
        "data": true
      },
      "f": true,
      "g": 0x7272727272727272,
      "h": {
        "data": enum1.e1_field0
      },
      "i": {
        "data": 123.456
      },
      "j": "world"
    },
    {
      "a": "foo",
      "b": true,
      "c": 0xaa,
      "d": enum1.e1_field0,
      "e": {
        "data": false
      },
      "f": false,
      "g": 0x12345678,
      "h": {
        "data": enum1.e1_field2
      },
      "i": {
        "data": 321.87
      },
      "j": "bar"
    },
    {
      "a": "some string",
      "b": true,
      "c": 0x18,
      "d": enum1.e1_field2,
      "e": {
        "data": true
      },
      "i": {
        "data": 123.456
      },
      "j": "another string"
    }
  ]
}

struct variable_nested_map {
  1: map<string, variable_nested> data
}

const variable_nested_map variable_nested_map_c = {
  "data": {
    "first": {
      "a": "hello",
      "b": false,
      "c": 0x91,
      "d": enum1.e1_field1,
      "e": {
        "data": true
      },
      "f": true,
      "g": 0x7272727272727272,
      "h": {
        "data": enum1.e1_field0
      },
      "i": {
        "data": 123.456
      },
      "j": "world"
    },
    "second": {
      "a": "foo",
      "b": true,
      "c": 0xaa,
      "d": enum1.e1_field0,
      "e": {
        "data": false
      },
      "f": false,
      "g": 0x12345678,
      "h": {
        "data": enum1.e1_field2
      },
      "i": {
        "data": 321.87
      },
      "j": "bar"
    },
    "third": {
      "a": "some string",
      "b": true,
      "c": 0x18,
      "d": enum1.e1_field2,
      "e": {
        "data": true
      },
      "i": {
        "data": 123.456
      },
      "j": "another string"
    }
  }
}

struct variable_nested_req_map {
  1: required map<string, variable_nested> data
}

const variable_nested_req_map variable_nested_req_map_c = {
  "data": {
    "first": {
      "a": "hello",
      "b": false,
      "c": 0x91,
      "d": enum1.e1_field1,
      "e": {
        "data": true
      },
      "f": true,
      "g": 0x7272727272727272,
      "h": {
        "data": enum1.e1_field0
      },
      "i": {
        "data": 123.456
      },
      "j": "world"
    },
    "second": {
      "a": "foo",
      "b": true,
      "c": 0xaa,
      "d": enum1.e1_field0,
      "e": {
        "data": false
      },
      "f": false,
      "g": 0x12345678,
      "h": {
        "data": enum1.e1_field2
      },
      "i": {
        "data": 321.87
      },
      "j": "bar"
    },
    "third": {
      "a": "some string",
      "b": true,
      "c": 0x18,
      "d": enum1.e1_field2,
      "e": {
        "data": true
      },
      "i": {
        "data": 123.456
      },
      "j": "another string"
    }
  }
}

struct variable_nested_hmap {
  1: hash_map<string, variable_nested> data
}

const variable_nested_hmap variable_nested_hmap_c = {
  "data": {
    "first": {
      "a": "hello",
      "b": false,
      "c": 0x91,
      "d": enum1.e1_field1,
      "e": {
        "data": true
      },
      "f": true,
      "g": 0x7272727272727272,
      "h": {
        "data": enum1.e1_field0
      },
      "i": {
        "data": 123.456
      },
      "j": "world"
    },
    "second": {
      "a": "foo",
      "b": true,
      "c": 0xaa,
      "d": enum1.e1_field0,
      "e": {
        "data": false
      },
      "f": false,
      "g": 0x12345678,
      "h": {
        "data": enum1.e1_field2
      },
      "i": {
        "data": 321.87
      },
      "j": "bar"
    },
    "third": {
      "a": "some string",
      "b": true,
      "c": 0x18,
      "d": enum1.e1_field2,
      "e": {
        "data": true
      },
      "i": {
        "data": 123.456
      },
      "j": "another string"
    }
  }
}

struct variable_nested_req_hmap {
  1: required hash_map<string, variable_nested> data
}

const variable_nested_req_hmap variable_nested_req_hmap_c = {
  "data": {
    "first": {
      "a": "hello",
      "b": false,
      "c": 0x91,
      "d": enum1.e1_field1,
      "e": {
        "data": true
      },
      "f": true,
      "g": 0x7272727272727272,
      "h": {
        "data": enum1.e1_field0
      },
      "i": {
        "data": 123.456
      },
      "j": "world"
    },
    "second": {
      "a": "foo",
      "b": true,
      "c": 0xaa,
      "d": enum1.e1_field0,
      "e": {
        "data": false
      },
      "f": false,
      "g": 0x12345678,
      "h": {
        "data": enum1.e1_field2
      },
      "i": {
        "data": 321.87
      },
      "j": "bar"
    },
    "third": {
      "a": "some string",
      "b": true,
      "c": 0x18,
      "d": enum1.e1_field2,
      "e": {
        "data": true
      },
      "i": {
        "data": 123.456
      },
      "j": "another string"
    }
  }
}

struct amalgamation {
  1: bool a
  2: required byte b
  3: optional enum1 c
  4: struct_bool d
  5: required nested e
  6: optional bool f
  7: i64 g
  8: required struct_e1 h
  9: optional struct_dbl i
  10: struct_str j
  11: required struct_lst_i32 k
  12: optional struct_set_i32 l
  13: struct_map_i32_str m
  14: required struct_hmap_i32_str n
  15: optional int_fields o
  16: basic_fields p
  17: required multi_fields_sparse q
  18: variable_nested r
  19: optional padded_booleans s
  20: unpadded_booleans t
}

const amalgamation amalgamation_c = {
  "a": true,
  "b": 0x34,
  "c": enum1.e1_field2,
  "d": {
    "data": false
  },
  "e": {
    "a": false,
    "b": 0x91,
    "c": enum1.e1_field1,
    "d": {
      "data": true
    },
    "e": true,
    "f": 0x7272727272727272,
    "g": {
      "data": enum1.e1_field0
    },
    "h": {
      "data": 123.456
    }
  },
  "f": false,
  "g": 0x2727272727272727,
  "h": {
    "data": enum1.e1_field0
  },
  "i": {
    "data": 987.21
  },
  "j": {
    "data": "hello, world"
  },
  "k": {
    "data": [ 321, 432, 543, 654, 765, 876, 987 ]
  },
  "l": {
    "data": [ 13, 24, 35, 46, 57 ]
  },
  "m": {
    "data": {
      333: "qwer",
      444: "tyuio",
      555: "asdfgh",
      666: "zxcvbnm"
    }
  },
  "n": {
    "data": {
      21: "212120",
      32: "323202",
      43: "434043",
      54: "540454",
      65: "606565"
    }
  },
  "o": {
    "b1": 0,
    "b2": 1,
    "y3": 0x53,
    "b4": 0,
    "s5": 0x5555,
    "i6": 0x56565656,
    "s7": 0x5757,
    "y8": 0x58,
    "l9": 0x5959595959595959,
    "y10": 0x60
  },
  "p": {
    "b1": 1,
    "b2": 0,
    "y3": 0x53,
    "b4": 1,
    "s5": 0x5555,
    "i6": 0x56565656,
    "s7": 0x5757,
    "y8": 0x58,
    "l9": 0x5959595959595959,
    "y10": 0x60,
    "z11": "\x61\x61\x61\x61\x61"
  },
  "q": {
    "b1": 1,
    "b2": 0,
    "y3": 0x53,
    "b4": 1,
    "s5": 0x5555,
    "i6": 0x56565656,
    "s7": 0x5757,
    "y8": 0x58,
    "l9": 0x5959595959595959,
    "y10": 0x60,
    "z11": "\x61\x61\x61\x61\x61",
    "b22": 1,
    "b23": 0,
    "y24": 0x63,
    "b25": 1,
    "s26": 0x6565,
    "i27": 0x66666666,
    "s28": 0x6767,
    "y29": 0x68,
    "l30": 0x6969696969696969,
    "y31": 0x70,
    "z32": "\x71\x71\x71\x71\x71\x71\x71",
    "b43": 1,
    "b44": 0,
    "y45": 0x73,
    "b46": 1,
    "s47": 0x7575,
    "i48": 0x76767676,
    "s49": 0x7777,
    "y50": 0x78,
    "l51": 0x7979797979797979,
    "y52": 0x80,
    "z53": "\x81\x81"
  },
  "r": {
    "a": "hello",
    "b": false,
    "c": 0x91,
    "d": enum1.e1_field1,
    "e": {
      "data": true
    },
    "f": true,
    "g": 0x7272727272727272,
    "h": {
      "data": enum1.e1_field0
    },
    "i": {
      "data": 123.456
    },
    "j": "world"
  }
  "s": {
    "b1": true,
    "b2": false,
    "b5": true,
    "b7": true,
    "b8": true,
    "b11": false,
    "b13": true,
    "b15": false,
    "b16": false,
    "b17": true,
    "b20": true,
    "b21": false
  },
  "t": {
    "b1": true,
    "b2": false,
    "b5": true,
    "b7": true,
    "b8": true,
    "b11": false,
    "b13": true,
    "b15": false,
    "b16": false,
    "b17": true,
    "b20": true,
    "b21": false
  }
}
