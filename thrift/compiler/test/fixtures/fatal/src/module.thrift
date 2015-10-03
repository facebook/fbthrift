namespace cpp2 test_cpp2.cpp_reflection

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
}

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

struct structB {
  1: double c
  2: bool d
}

struct struct1 {
  1: i32 field0
  2: string field1
  3: enum1 field2
  4: enum2 field3
  5: union1 field4
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
