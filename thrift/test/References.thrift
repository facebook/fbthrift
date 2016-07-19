namespace cpp2 cpp2

struct RefStruct {
  1:          RefStruct def_field (cpp2.ref = "true")
  2: optional RefStruct opt_field (cpp2.ref = "true")
  3: required RefStruct req_field (cpp2.ref = "true")
}
