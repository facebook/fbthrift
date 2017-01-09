namespace cpp apache.thrift.test
namespace cpp2 apache.thrift.test

struct Empty {}

struct StructWithRef {
  1:          Empty def_field (cpp.ref),
  2: optional Empty opt_field (cpp.ref),
  3: required Empty req_field (cpp.ref),
}

const StructWithRef kStructWithRef = {
  "def_field": {},
  "opt_field": {},
  "req_field": {},
}

struct StructWithRefTypeUnique {
  1:          Empty def_field (cpp.ref_type = "unique"),
  2: optional Empty opt_field (cpp.ref_type = "unique"),
  3: required Empty req_field (cpp.ref_type = "unique"),
}

const StructWithRefTypeUnique kStructWithRefTypeUnique = {
  "def_field": {},
  "opt_field": {},
  "req_field": {},
}

struct StructWithRefTypeShared {
  1:          Empty def_field (cpp.ref_type = "shared"),
  2: optional Empty opt_field (cpp.ref_type = "shared"),
  3: required Empty req_field (cpp.ref_type = "shared"),
}

const StructWithRefTypeShared kStructWithRefTypeShared = {
  "def_field": {},
  "opt_field": {},
  "req_field": {},
}

struct StructWithRefTypeSharedConst {
  1:          Empty def_field (cpp.ref_type = "shared_const"),
  2: optional Empty opt_field (cpp.ref_type = "shared_const"),
  3: required Empty req_field (cpp.ref_type = "shared_const"),
}

const StructWithRefTypeSharedConst kStructWithRefTypeSharedConst = {
  "def_field": {},
  "opt_field": {},
  "req_field": {},
}

struct StructWithRefTypeCustom {
  1:          Empty def_field (cpp.ref_type = "std::unique_ptr"),
  2: optional Empty opt_field (cpp.ref_type = "std::unique_ptr"),
  3: required Empty req_field (cpp.ref_type = "std::unique_ptr"),
}

const StructWithRefTypeCustom kStructWithRefTypeCustom = {
  "def_field": {},
  "opt_field": {},
  "req_field": {},
}
