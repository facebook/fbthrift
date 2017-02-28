namespace py my.namespacing.test.module
namespace py3 my.namespacing.test.module

struct Foo {
  1: i64 MyInt;
}

service TestService {
  i64 init(
    1: i64 int1,
  )
}
