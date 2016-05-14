union MyUnion {
  1: i32 anInteger,
  2: string aString,
}

struct MyField {
  1: optional i64 opt_value,
  2: i64 value,
  3: required i64 req_value,
}

struct MyStruct {
  1: optional MyField opt_ref (cpp.ref = "true", cpp2.ref = "true")
  2: MyField ref (cpp.ref = "true", cpp2.ref = "true")
  3: required MyField req_ref (cpp.ref = "true", cpp2.ref = "true")
}

struct StructWithUnion {
  1: MyUnion u (cpp.ref = "true"),
  2: double aDouble,
  3: MyField f,
}

struct RecursiveStruct {
  1: optional list<RecursiveStruct> mes,
}

struct StructWithContainers {
  1: list<i32> list_ref (cpp.ref = "true", cpp2.ref = "true")
  2: set<i32> set_ref (cpp.ref = "true", cpp2.ref = "true")
  3: map<i32, i32> map_ref (cpp.ref = "true", cpp2.ref = "true")
  4: list<i32> list_ref_unique
      (cpp.ref_type = "unique", cpp2.ref_type = "unique")
  5: set<i32> set_ref_shared (cpp.ref_type = "shared", cpp2.ref_type = "shared")
  6: map<i32, i32> (cpp.type = "const std::map<int32_t, int32_t>")
      map_ref_custom (cpp.ref_type = "shared", cpp2.ref_type = "shared")
  7: list<i32> list_ref_shared_const
      (cpp.ref_type = "shared_const", cpp2.ref_type = "shared_const")
  8: set<i32> set_custom_ref
      (cpp.ref_type = "std::auto_ptr", cpp2.ref_type = "std::auto_ptr")
}

enum TypedEnum {
  VAL1,
  VAL2,
} (cpp.enum_type = "short")
