namespace cpp test_cpp
namespace cpp2 test_cpp2

enum enum1 {
  field0,
  field1,
  field2
}

const enum1 e_1 = field0
const enum1 e_2 = field2

const i32 i_1 = 72;
const i32 i_2 = 99;

const string str_1 = "hello"
const string str_2 = "world"
const string str_3 = "'"
const string str_4 = '"foo"'

const list<i32> l_1 = [23, 42, 56]
const list<string> l_2 = ["foo", "bar", "baz"]

const set<i32> s_1 = [23, 42, 56]
const set<string> s_2 = ["foo", "bar", "baz"]

const map<i32, i32> m_1 = {23:97, 42:37, 56:11}
const map<string, string> m_2 = {"foo":"bar", "baz":"gaz"}
const map<string, i32> m_3 = {'"': 34, "'": 39, "\\": 92, "\x61": 97}

struct struct1 {
  1: i32 a = 1234567
  2: string b = "<uninitialized>"
}

const struct1 pod_0 = {};

const struct1 pod_1 = {
  "a":10,
  "b":"foo"
}

struct struct2 {
  1: i32 a
  2: string b
  3: struct1 c
  4: list<i32> d
}

const struct2 pod_2 = {
  "a":98,
  "b":"gaz",
  "c":{
    "a":12,
    "b":"bar"
  },
  "d":[11, 22, 33]
}
