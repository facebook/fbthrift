namespace cpp2 cpp2

struct RefStruct {
  1:          RefStruct def_field (cpp2.ref = "true")
  2: optional RefStruct opt_field (cpp2.ref = "true")
  3: required RefStruct req_field (cpp2.ref = "true")
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
      (cpp.ref_type = "std::unique_ptr", cpp2.ref_type = "std::unique_ptr")
}
