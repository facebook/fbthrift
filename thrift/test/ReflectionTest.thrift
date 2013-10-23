/**
 * Copyright 2012 Facebook
 * @author Tudor Bosman (tudorb@fb.com)
 */

namespace cpp thrift.test

struct ReflectionTestStruct1 {
  3: i32 c,
  1: required i32 a,
  2: optional i32 b,
  4: string d (some.field.annotation = "hello",
               some.other.annotation = 1,
               annotation.without.value),
}

enum ReflectionTestEnum {
  FOO = 5,
  BAR = 4,
}

struct ReflectionTestStruct2 {
  1: map<byte, ReflectionTestStruct1> a,
  2: set<string> b,
  3: list<i64> c,
  4: ReflectionTestEnum d,
}

