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
