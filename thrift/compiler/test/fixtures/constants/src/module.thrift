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

enum EmptyEnum {}

enum City { NYC, MPK, SEA, LON }
enum Company { FACEBOOK, WHATSAPP, OCULUS, INSTAGRAM }

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

const bool false_c = 0;
const bool true_c = 1;
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
