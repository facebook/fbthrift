namespace cpp2 static_reflection.demo

enum annotated_enum {
  field0,
  field1,
  field2,
} (
  description = "example of an annotated enum",
  purpose = "toy example of enum annotations"
)

struct annotated_struct {
  1: i32 i32_data (
    description = "example of an annotated struct member"
  )
  2: i16 i16_data
  3: double double_data
  4: string string_data (
    description = "example of a fourth annotated struct member"
  )
} (
  description = "example of an annotated struct",
  purpose = "toy example of struct annotations"
)
