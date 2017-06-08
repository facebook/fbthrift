include "includes.thrift"
cpp_include "<folly/small_vector.h>"

namespace cpp2 some.valid.ns

// Generate base consts
const bool aBool = true
const byte aByte = 1
const i16 a16BitInt = 12
const i32 a32BitInt = 123
const i64 a64BitInt = 1234
const float aFloat = 0.1
const double aDouble = 0.12
const string aString = "Joe Doe"
const list<bool> aList = [true, false]
const map<i32, string> aMap = {
  1: "foo",
  2: "bar",
}
const set<string> aSet = ["foo", "bar"]
const list<list<i32>> aListOfLists = [
  [1, 3, 5, 7, 9],
  [2, 4, 8, 10, 12]
]
const list<map<string, i32>> states = [
  {"San Diego": 3211000, "Sacramento": 479600, "SF": 837400},
  {"New York": 8406000, "Albany": 98400}
]

enum MyEnumA {
  fieldA = 1
  fieldB = 2
  fieldC = 4
}

const MyEnumA constEnumA = MyEnumA.fieldB

const MyEnumA constEnumB = 3

struct Empty {
}

struct MyStruct {
  1: bool MyBoolField,
  2: i64 MyIntField = 12,
  3: string MyStringField = "test"
  4: string MyStringField2
}

union SimpleUnion {
  7: i64 intValue;
  2: string stringValue;
}

typedef i32 simpleTypeDef
typedef map<i16, string> containerTypeDef
typedef list<map<i16, string>> complexContainerTypeDef
typedef set<SimpleUnion> unionTypeDef
typedef list<MyStruct> structTypeDef
typedef list<map<Empty, MyStruct>> complexStructTypeDef
typedef list<complexStructTypeDef> mostComplexTypeDef

union ComplexUnion {
  1: i64 intValue;
  101: required i64 req_intValue;
  201: optional i64 opt_intValue;
  3: string stringValue;
  103: required string req_stringValue;
  203: optional string opt_stringValue;
  4: i16 intValue2;
  6: i32 intValue3;
  7: double doubelValue;
  8: bool boolValue;
  9: list<i32> union_list
  10: set<i64> union_set
  11: map<string, i32> union_map
  111: required map<string, i32> req_union_map
  211: optional map<string, i32> opt_union_map
  12: MyEnumA enum_field
  13: list<MyEnumA> enum_container
  14: MyStruct a_struct
  15: set<MyStruct> a_set_struct
  16: SimpleUnion a_union
  116: required SimpleUnion req_a_union
  216: optional SimpleUnion opt_a_union
  17: list<SimpleUnion> a_union_list
  18: unionTypeDef a_union_typedef
  19: list<unionTypeDef> a_union_typedef_list
}

exception AnException {
  1: i32 code
  101: required i32 req_code
  2: string message
  102: required string req_message
  3: list<i32> exception_list = [1, 2, 3]
  4: set<i64> exception_set
  5: map<string, i32> exception_map
  105: required map<string, i32> req_exception_map
  6: MyEnumA enum_field
  7: list<MyEnumA> enum_container
  8: MyStruct a_struct
  9: set<MyStruct> a_set_struct
  10: list<SimpleUnion> a_union_list
  11: unionTypeDef union_typedef
  19: list<unionTypeDef> a_union_typedef_list
}

struct containerStruct {
  1: bool fieldA
  101: required bool req_fieldA
  201: optional bool opt_fieldA
  2: map<string, bool> fieldB
  102: required map<string, bool> req_fieldB
  202: optional map<string, bool> opt_fieldB
  3: set<i32> fieldC = [1, 2, 3, 4]
  103: required set<i32> req_fieldC = [1, 2, 3, 4]
  203: optional set<i32> opt_fieldC = [1, 2, 3, 4]
  4: string fieldD
  5: string fieldE = "somestring"
  105: required string req_fieldE = "somestring"
  205: optional string opt_fieldE = "somestring"
  6: list<list<i32>> fieldF = aListOfLists
  7: map<string, map<string, map<string, i32>>> fieldG
  8: list<set<i32>> fieldH
  9: bool fieldI = true
  10: map<string, list<i32>> fieldJ = {
       "subfieldA" : [1, 4, 8, 12],
       "subfieldB" : [2, 5, 9, 13],
     }
  11: list<list<list<list<i32>>>> fieldK
  12: set<set<set<bool>>> fieldL
  13: map<set<list<i32>>, map<list<set<string>>, string>> fieldM
  14: simpleTypeDef fieldN
  15: complexStructTypeDef fieldO
  16: list<mostComplexTypeDef> fieldP
  17: MyEnumA fieldQ
  18: MyEnumA fieldR = MyEnumA.fieldB
  118: required MyEnumA req_fieldR = MyEnumA.fieldB
  218: optional MyEnumA opt_fieldR = MyEnumA.fieldB
  19: MyEnumA fieldS = constEnumA
  21: list<MyEnumA> fieldT
  22: list<MyEnumA> fieldU = [MyEnumA.fieldC, MyEnumA.fieldB, MyEnumA.fieldA]
  23: MyStruct fieldV
  123: required MyStruct req_fieldV
  223: optional MyStruct opt_fieldV
  24: set<MyStruct> fieldW
  25: ComplexUnion fieldX
  125: required ComplexUnion req_fieldX
  225: optional ComplexUnion opt_fieldX
  26: list<ComplexUnion> fieldY
  27: unionTypeDef fieldZ
  28: list<unionTypeDef> fieldAA
}

struct MyIncludedStruct {
  1: includes.IncludedInt64 MyIncludedInt = includes.IncludedConstant
}

service EmptyService {
}

service ReturnService {
  void noReturn()
  bool boolReturn()
  i16 i16Return()
  i32 i32Return()
  i64 i64Return()
  float floatReturn()
  double doubleReturn()
  string stringReturn()
  map<string, i64> mapReturn()
  simpleTypeDef simpleTypedefReturn()
  complexStructTypeDef complexTypedefReturn()
  list<mostComplexTypeDef> list_mostComplexTypedefReturn()
  MyEnumA enumReturn()
  list<MyEnumA> list_EnumReturn()
  MyStruct structReturn()
  set<MyStruct> set_StructReturn()
  ComplexUnion unionReturn()
  list<ComplexUnion> list_UnionReturn()
}

service ParamService {
  void void_ret_i16_param(1: i16 param1)
  void void_ret_byte_i16_param(1: byte param1, 2: i16 param2)
  bool bool_ret_i32_i64_param(1: i32 param1, 3: i64 param2)
  i64 i64_ret_float_double_param(1: float param1, 3: double param2)
  string string_ret_string_param(1: string param1)
  map<string, i64> map_ret_bool_param(1: bool param1)
  bool bool_ret_map_param(1: map<string, i64> param1)
  void void_ret_map_param(1: map<string, i64> param1)
}
