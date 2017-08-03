namespace java.swift test.fixtures.constants

const i32 myInt = 1337;
const string name = "Mark Zuckerberg";
const list<map<string, i32>> states = [
  {"San Diego": 3211000, "Sacramento": 479600, "SF": 837400},
  {"New York": 8406000, "Albany": 98400}
];
const double x = 1.0;
const double y = 1000000;
const double z = 1000000000.0;
const double zeroDoubleValue = 0.0;

const double longDoubleValue = 0.0000259961000990301;

enum EmptyEnum {}

enum City {
  NYC = 0,
  MPK = 1,
  SEA = 2,
  LON = 3,
}
enum Company {
  FACEBOOK = 0,
  WHATSAPP = 1,
  OCULUS = 2,
  INSTAGRAM = 3,
}

struct Internship {
  1: required i32 weeks;
  2: string title;
  3: optional Company employer;
}

const Internship instagram = {
  "weeks": 12,
  "title": "Software Engineer",
  "employer": Company.INSTAGRAM
};

struct UnEnumStruct {
  1: City city = -1, # thrift-compiler should emit a warning
}

struct Range {
  1: required i32 min;
  2: required i32 max;
}

const list<Range> kRanges = [
  {
    "min": 1,
    "max": 2,
  },
  {
    "min": 5,
    "max": 6,
  },
]

const list<Internship> internList = [
  instagram,
  {
    "weeks": 10,
    "title": "Sales Intern",
    "employer": Company.FACEBOOK
  }
];

struct struct1 {
  1: i32 a = 1234567
  2: string b = "<uninitialized>"
}

const struct1 pod_0 = {};

const struct1 pod_1 = {
  "a": 10,
  "b": "foo"
}

struct struct2 {
  1: i32 a
  2: string b
  3: struct1 c
  4: list<i32> d
}

const struct2 pod_2 = {
  "a": 98,
  "b": "gaz",
  "c": {
    "a": 12,
    "b": "bar"
  },
  "d": [11, 22, 33]
}

struct struct3 {
  1: string a
  2: i32 b
  3: struct2 c
}

const struct3 pod_3 = {
  "a":"abc",
  "b":456,
  "c": {
    "a":888,
    "c":{
      "b":"gaz"
    }
    "d": [1, 2, 3]
  }
}

union union1 {
  1: i32 i
  2: double d
}

const union1 u_1_1 = {
  "i": 97
}

const union1 u_1_2 = {
  "d": 5.6
}

const union1 u_1_3 = {}

union union2 {
  1: i32 i
  2: double d
  3: struct1 s
  4: union1 u
}

const union2 u_2_1 = {
  "i": 51
}

const union2 u_2_2 = {
  "d": 6.7
}

const union2 u_2_3 = {
  "s": {
    "a": 8,
    "b": "abacabb"
  }
}

const union2 u_2_4 = {
  "u": {
    "i": 43
  }
}

const union2 u_2_5 = {
  "u": {
    "d": 9.8
  }
}

const union2 u_2_6 = {
  "u": {}
}

const string apostrophe = "'";
const string tripleApostrophe = "'''";
const string quotationMark = '"'; //" //fix syntax highlighting
const string backslash = "\\";
const string escaped_a = "\x61";

const map<string, i32> char2ascii = {
  "'": 39,
  '"': 34,
  "\\": 92,
  "\x61": 97,
};

const list<string> escaped_strings = [
  "\x61", "\xab", "\x6a", "\xa6",
  "\x61yyy", "\xabyyy", "\x6ayyy", "\xa6yyy",
  "zzz\x61", "zzz\xab", "zzz\x6a", "zzz\xa6",
  "zzz\x61yyy", "zzz\xabyyy", "zzz\x6ayyy", "zzz\xa6yyy"
];

const bool false_c = false;
const bool true_c = true;
const byte zero_byte = 0;
const i16 zero16 = 0;
const i32 zero32 = 0;
const i64 zero64 = 0;
const double zero_dot_zero = 0.0;
const string empty_string = "";
const list<i32> empty_int_list = {};
const list<string> empty_string_list = {};
const set<i32> empty_int_set = {};
const set<string> empty_string_set = {};
const map<i32, i32> empty_int_int_map = {};
const map<i32, string> empty_int_string_map = {};
const map<string, i32> empty_string_int_map = {};
const map<string, string> empty_string_string_map = {};
