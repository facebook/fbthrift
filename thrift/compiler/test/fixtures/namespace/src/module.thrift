namespace py my.namespacing.test

struct Foo {
  1: i64 MyInt;
}

service TestService {
  i64 init(
    1: i64 int1,
  )
}
