/*
 * Copyright 2013 Facebook
 * @author Dmytro Dzhulgakov (dzhulgakov@fb.com)
 */

namespace cpp thrift.test.serialized_fields
namespace java thrift.test

cpp_include "<unordered_set>"
cpp_include "<unordered_map>"

struct SubStruct {
  1: i64 id,
  2: string value,
}

view SubStructView : SubStruct {
  id,
}

struct MetaStruct {
  1: byte a_bite,
  2: i64 integer64,
  3: string str,
  4: list<i64> i64_list,
  5: map<string, string> mapping,
  6: set<string> setting,
  7: SubStruct substruct,
  8: list<SubStruct> substructs,
}

view DifferentImplStruct : MetaStruct {
  4: list<i64> i64_list,
  5: map<string, string> (cpp.template = "std::unordered_map") mapping,
  6: set<string> (cpp.template = "std::unordered_set") setting,
  7: SubStructView substruct,
  8: list<SubStructView> substructs,
}

view SomeFieldsSerialized : MetaStruct {
  a_bite,
  integer64 (format = "serialized"),
  //str, field will be dropped
  i64_list,
  mapping (format = "serialized"),
  //setting, field will be dropped
  substruct,
  substructs (format = "serialized"),
}

view ProxyUnknownStruct : MetaStruct {
  1: byte a_bite,
  3: string str,
} (
  keep_unknown_fields = 1
)

const MetaStruct kTestMetaStruct = {
  'a_bite': 123,
  'integer64': 4567890,
  'str': 'Some fancy text',
  'i64_list': [678, 567, 456, 987],
  'mapping': {'Bob': 'Apple', 'John': 'Orange'},
  'setting': ['Foo', 'Bar', 'FooBar'],
  'substruct': {'id': 55555555, 'value': 'qwerty'},
  'substructs': [
    {'id': 66666666, 'value': 'foo'},
    {'id': 77777777, 'value': 'bar'},
    {'id': 88888888, 'value': 'foobar'}
  ]
}

const ProxyUnknownStruct kTestProxyUnknownStruct = {
  'a_bite': 123,
  'str': 'Some fancy text',
}

struct ParserTest {
  1: i32 view,
}

service ParserTestService {
  void foo(1: i32 arg, 2: byte arg2);
}
