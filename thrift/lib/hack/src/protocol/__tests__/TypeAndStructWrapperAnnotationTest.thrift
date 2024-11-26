include "thrift/annotation/hack.thrift"

package "meta.com/thrift/wrapper_test"

namespace hack "WrapperTest"

@hack.Wrapper{name = "\MyStructWrapper"}
struct StructWithWrapper_ {
  1: i64 int_field;
}

@hack.Wrapper{name = "\MyTypeIntWrapper"}
typedef i64 i64WithWrapper_

struct structured_annotation_with_default {
  1: i32 count = 0;
}

struct structured_annotation_recursive {
  1: string name;
  @hack.FieldWrapper{name = "\MyFieldWrapper"}
  2: structured_annotation_recursive recurse;
  3: structured_annotation_with_default default;
  4: map<string, structured_annotation_recursive> recurse_map;
  5: i64WithWrapper_ int_field;
}

@structured_annotation_recursive{
  name = "abc_struct",
  recurse = structured_annotation_recursive{name = "cba_struct"},
  default = structured_annotation_with_default{count = 3},
  recurse_map = {
    'key_struct1': structured_annotation_recursive{
      name = "key_struct1",
      recurse = structured_annotation_recursive{name = "def_struct"},
    },
    'key_struct2': structured_annotation_recursive{
      name = "key_struct2",
      recurse = structured_annotation_recursive{name = "fed_struct"},
    },
  },
  int_field = 10,
}
struct AnnotationTestMyStruct {

  @structured_annotation_recursive{
    name = "abc_struct_field",
    recurse = structured_annotation_recursive{name = "cba_struct_field"},
    default = structured_annotation_with_default{count = 3},
    recurse_map = {
      'key_struct_field1': structured_annotation_recursive{
        name = "key_struct_field1",
        recurse = structured_annotation_recursive{name = "def_struct_field"},
      },
      'key_struct_field2': structured_annotation_recursive{
        name = "key_struct_field2",
        recurse = structured_annotation_recursive{name = "fed_struct_field"},
      },
    },
  }
  1: string annotated_recursive;
  @StructWithWrapper_{int_field = 11}
  2: string annotated_struct_wrapper;
}

@structured_annotation_recursive{
  name = "abc_service",
  recurse = structured_annotation_recursive{name = "cba_service"},
  default = structured_annotation_with_default{count = 3},
  recurse_map = {
    'key_service1': structured_annotation_recursive{
      name = "key_service1",
      recurse = structured_annotation_recursive{name = "def_service"},
    },
    'key_service2': structured_annotation_recursive{
      name = "key_service2",
      recurse = structured_annotation_recursive{name = "fed_service"},
    },
  },
}
service MyService {
  @structured_annotation_recursive{
    name = "abc_service_method",
    recurse = structured_annotation_recursive{name = "cba_service_method"},
    default = structured_annotation_with_default{count = 3},
    recurse_map = {
      'key_service_method1': structured_annotation_recursive{
        name = "key_service_method1",
        recurse = structured_annotation_recursive{name = "def_service_method"},
      },
      'key_servic_methode2': structured_annotation_recursive{
        name = "key_service_method2",
        recurse = structured_annotation_recursive{name = "fed_service_method"},
      },
    },
  }
  bool second(
    @structured_annotation_recursive{
      name = "abc_service_method",
      recurse = structured_annotation_recursive{name = "cba_service_method"},
      default = structured_annotation_with_default{count = 3},
      recurse_map = {
        'key_service_method1': structured_annotation_recursive{
          name = "key_service_method1",
          recurse = structured_annotation_recursive{
            name = "def_service_method",
          },
        },
        'key_servic_methode2': structured_annotation_recursive{
          name = "key_service_method2",
          recurse = structured_annotation_recursive{
            name = "fed_service_method",
          },
        },
      },
    }
    1: i64 count,
  );
}

enum MyEnum {

  @structured_annotation_recursive{
    name = "abc_enum",
    recurse = structured_annotation_recursive{name = "cba_enum"},
    default = structured_annotation_with_default{count = 3},
    recurse_map = {
      'key_enum1': structured_annotation_recursive{
        name = "key_enum1",
        recurse = structured_annotation_recursive{name = "def_enum"},
      },
      'key_enum2': structured_annotation_recursive{
        name = "key_enum2",
        recurse = structured_annotation_recursive{name = "fed_enum"},
      },
    },
  }
  UNKNOWN = 0,
  ONE = 1,
}

@structured_annotation_recursive{
  name = "abc_constants",
  recurse = structured_annotation_recursive{name = "cba_constants"},
  default = structured_annotation_with_default{count = 3},
  recurse_map = {
    'key_constants1': structured_annotation_recursive{
      name = "key_constants1",
      recurse = structured_annotation_recursive{name = "def_constants"},
    },
    'key_constants2': structured_annotation_recursive{
      name = "key_constants2",
      recurse = structured_annotation_recursive{name = "fed_constants"},
    },
  },
}
const map<string, string> MyConst = {'ENUMERATOR': 'enum', 'CONST': 'const'};
