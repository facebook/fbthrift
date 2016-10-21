namespace cpp test_cpp1.cpp_reflection
namespace cpp2 test_cpp2.cpp_reflection
namespace d test_d.cpp_reflection
namespace java test_java.cpp_reflection
namespace java.swift test_swift.cpp_reflection
namespace php test_php.cpp_reflection
namespace python test_py.cpp_reflection

cpp_include "thrift/test/fatal_custom_types.h"

enum enum1 {
  field0,
  field1,
  field2
}

enum enum2 {
  field0_2,
  field1_2,
  field2_2
}

enum enum3 {
  field0_3,
  field1_3,
  field2_3
} (
  one.here = "with some value associated",
  another.there = ".",
  yet.another = "and yet more text - it's that easy",
  duplicate_id_annotation_1 = "duplicate id annotation",
  duplicate_id_annotation_2 = "duplicate.id.annotation",
  _now.with.an.underscore = "_now.with.an.underscore"
)

union union1 {
  1: i32 ui
  2: double ud
  3: string us
  4: enum1 ue
}

union union2 {
  1: i32 ui_2
  2: double ud_2
  3: string us_2
  4: enum1 ue_2
}

union union3 {
  1: i32 ui_3
  2: double ud_3
  3: string us_3
  4: enum1 ue_3
}

struct structA {
  1: i32 a
  2: string b
}

typedef structA (
  cpp.type = "test_cpp_reflection::custom_structA"
) my_structA

union unionA {
  1: i32 i
  2: double d
  3: string s
  4: enum1 e
  5: structA a
} (
  sample.annotation = "some text here",
  another.annotation = "some more text",
)

struct structB {
  1: double c
  2: bool d (
    some.annotation = "some value",
    another.annotation = "another value",
  )
} (
  some.annotation = "this is its value",
  some.other.annotation = "this is its other value",
  multi_line_annotation = "line one
line two"
)

struct structC {
  1: i32 a
  2: string b
  3: double c
  4: bool d
  5: enum1 e
  6: enum2 f
  7: union1 g
  8: unionA h
  9: unionA i
  10: list<i32> j
  11: list<i32> j1
  12: list<enum1> j2
  13: list<structA> j3
  14: set<i32> k
  15: set<i32> k1
  16: set<enum2> k2
  17: set<structB> k3
  18: map<i32, i32> l
  19: map<i32, i32> l1
  20: map<i32, enum1> l2
  21: map<i32, structB> l3
  22: map<enum1, i32> m1
  23: map<enum1, enum2> m2
  24: map<enum1, structB> m3
  25: map<string, i32> n1
  26: map<string, enum1> n2
  27: map<string, structB> n3
  28: map<structA, i32> o1
  29: map<structA, enum1> o2
  30: map<structA, structB> o3
}

struct struct1 {
  1: required i32 field0
  2: optional string field1
  3: enum1 field2
  4: required enum2 field3
  5: optional union1 field4
  6: union2 field5
}

struct struct2 {
  1: i32 fieldA
  2: string fieldB
  3: enum1 fieldC
  4: enum2 fieldD
  5: union1 fieldE
  6: union2 fieldF
  7: struct1 fieldG
}

struct struct3 {
  1: i32 fieldA
  2: string fieldB
  3: enum1 fieldC
  4: enum2 fieldD
  5: union1 fieldE
  6: union2 fieldF
  7: struct1 fieldG
  8: union2 fieldH
  9: list<i32> fieldI
  10: list<string> fieldJ
  11: list<string> fieldK
  12: list<structA> fieldL
  13: set<i32> fieldM
  14: set<string> fieldN
  15: set<string> fieldO
  16: set<structB> fieldP
  17: map<string, structA> fieldQ
  18: map<string, structB> fieldR
}

struct struct4 {
  1: required i32 field0
  2: optional string field1
  3: enum1 field2
  6: structA field3 (cpp2.ref = "true")
}

struct struct5 {
  1: required i32 field0
  2: optional string field1
  3: enum1 field2
  4: structA field3 (annotate_here = "with text")
  5: structB field4
}

struct struct_binary {
  1: binary bi
}

struct annotated {
  1: i32 a (
    m_b_false = 'false',
    m_b_true = 'true',
    m_int = '10',
    m_string = '"hello"',
    m_int_list = '[-1, 2, 3]',
    m_str_list = '["a", "b", "c"]',
    m_mixed_list = '["a", 1, "b", 2]',
    m_int_map = '{"a": 1, "b": -2, "c": -3}',
    m_str_map = '{"a": "A", "b": "B", "c": "C"}',
    m_mixed_map = '{"a": -2, "b": "B", "c": 3}'
  )
} (
  s_b_false = 'false',
  s_b_true = 'true',
  s_int = '10',
  s_string = '"hello"',
  s_int_list = '[-1, 2, 3]',
  s_str_list = '["a", "b", "c"]',
  s_mixed_list = '["a", 1, "b", 2]',
  s_int_map = '{"a": 1, "b": -2, "c": -3}',
  s_str_map = '{"a": "A", "b": "B", "c": "C"}',
  s_mixed_map = '{"a": -2, "b": "B", "c": 3}'
)

service service1 {
  void method1();
  void method2(1: i32 x, 2: struct1 y, 3: double z);
  i32 method3();
  i32 method4(1: i32 i, 2: struct1 j, 3: double k);
  struct2 method5();
  struct2 method6(1: i32 l, 2: struct1 m, 3: double n);
}

service service2 {
  void methodA();
  void methodB(1: i32 x, 2: struct1 y, 3: double z);
  i32 methodC();
  i32 methodD(1: i32 i, 2: struct1 j, 3: double k);
  struct2 methodE();
  struct2 methodF(1: i32 l, 2: struct1 m, 3: double n);
}

service service3 {
  void methodA();
  void methodB(1: i32 x, 2: struct1 y, 3: double z);
  i32 methodC();
  i32 methodD(1: i32 i, 2: struct1 j, 3: double k);
  struct2 methodE();
  struct3 methodF(1: i32 l, 2: struct1 m, 3: double n);
}

const i32 constant1 = 1357;
const string constant2 = "hello";
const enum1 constant3 = enum1.field0;

enum enum_with_special_names {
  get,
  getter,
  lists,
  maps,
  name,
  name_to_value,
  names,
  prefix_tree,
  sets,
  setter,
  str,
  strings,
  type,
  value,
  value_to_name,
  values,
  id,
  ids,
  descriptor,
  descriptors,
  key,
  keys,
  annotation,
  annotations,
  member,
  members,
}

union union_with_special_names {
  1:  i32 get
  2:  i32 getter
  3:  i32 lists
  4:  i32 maps
  5:  i32 name
  6:  i32 name_to_value
  7:  i32 names
  8:  i32 prefix_tree
  9:  i32 sets
  10: i32 setter
  11: i32 str
  12: i32 strings
  13: i32 type
  14: i32 value
  15: i32 value_to_name
  16: i32 values
  17: i32 id
  18: i32 ids
  19: i32 descriptor
  20: i32 descriptors
  21: i32 key
  22: i32 keys
  23: i32 annotation
  24: i32 annotations
  25: i32 member
  26: i32 members
}

struct struct_with_special_names {
  1:  i32 get
  2:  i32 getter
  3:  i32 lists
  4:  i32 maps
  5:  i32 name
  6:  i32 name_to_value
  7:  i32 names
  8:  i32 prefix_tree
  9:  i32 sets
  10: i32 setter
  11: i32 str
  12: i32 strings
  13: i32 type
  14: i32 value
  15: i32 value_to_name
  16: i32 values
  17: i32 id
  18: i32 ids
  19: i32 descriptor
  20: i32 descriptors
  21: i32 key
  22: i32 keys
  23: i32 annotation
  24: i32 annotations
  25: i32 member
  26: i32 members
}

service service_with_special_names {
  i32 get()
  i32 getter()
  i32 lists()
  i32 maps()
  i32 name()
  i32 name_to_value()
  i32 names()
  i32 prefix_tree()
  i32 sets()
  i32 setter()
  i32 str()
  i32 strings()
  i32 type()
  i32 value()
  i32 value_to_name()
  i32 values()
  i32 id()
  i32 ids()
  i32 descriptor()
  i32 descriptors()
  i32 key()
  i32 keys()
  i32 annotation()
  i32 annotations()
  i32 member()
  i32 members()
}

const i32 constant_with_special_name = 42;
