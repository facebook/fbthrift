struct NoDefaults {
  1: required i64 req_field
  2: i64 unflagged_field
  3: optional i64 opt_field
}

struct WithDefaults {
  1: required i64 req_field = 10
  2: i64 unflagged_field = 20
  3: optional i64 opt_field = 30
}
