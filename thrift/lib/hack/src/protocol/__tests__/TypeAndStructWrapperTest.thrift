include "thrift/annotation/hack.thrift"
include "thrift/annotation/scope.thrift"

package "meta.com/thrift/wrapper_test"

namespace hack "WrapperTest"

@scope.Transitive
@hack.FieldWrapper{name = "\\MyFieldWrapper"}
struct AnnotationStruct {}

struct TestMyStruct {
  1: MyNestedStruct nested_struct;
}

typedef i64 i64WithAdapter

@hack.Wrapper{name = "\\MyTypeIntWrapper"}
typedef i64 i64WithWrapper

struct MyNestedStruct {
  @hack.FieldWrapper{name = "\\MyFieldWrapper"}
  1: i64 wrapped_field;
  @AnnotationStruct
  2: i64 annotated_field;
  @hack.Adapter{name = "\\AdapterTestIntToString"}
  3: i64 adapted_type;
  @hack.FieldWrapper{name = "\\MyFieldWrapper"}
  @hack.Adapter{name = "\\AdapterTestIntToString"}
  4: i64 adapted__and_wrapped_type;
  @hack.FieldWrapper{name = "\\MyFieldWrapper"}
  @hack.Adapter{name = "\\AdapterTestIntToString"}
  5: optional i64WithAdapter optional_adapted_and_wrapped_type;
  @hack.FieldWrapper{name = "\\MyFieldWrapper"}
  8: StructWithWrapper double_wrapped_struct;
}

struct MyComplexStruct {
  1: map<string, TestMyStruct> map_of_string_to_TestMyStruct;
  2: map<string, list<TestMyStruct>> map_of_string_to_list_of_TestMyStruct;
  3: map<string, map<string, i32>> map_of_string_to_map_of_string_to_i32;
  4: map<
    string,
    map<string, TestMyStruct>
  > map_of_string_to_map_of_string_to_TestMyStruct;
  5: list<
    map<string, list<TestMyStruct>>
  > list_of_map_of_string_to_list_of_TestMyStruct;
  6: list<map<string, TestMyStruct>> list_of_map_of_string_to_TestMyStruct;
  7: list<
    map<string, StructWithWrapper>
  > list_of_map_of_string_to_StructWithWrapper;
}

service Service1 {
  TestMyStruct func1(1: string arg1, 2: TestMyStruct arg2);
  i64WithWrapper func2(1: StructWithWrapper arg1, 2: i64WithWrapper arg2);
}

@hack.Wrapper{name = "\\MyStructWrapper"}
struct StructWithWrapper {
  1: i64 int_field;
}
