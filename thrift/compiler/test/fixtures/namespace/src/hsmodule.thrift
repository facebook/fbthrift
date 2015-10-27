namespace hs My.Namespacing.Test

struct HsFoo {
  1: i64 MyInt;
}

service HsTestService {
  i64 init(
    1: i64 int1,
  )
}
