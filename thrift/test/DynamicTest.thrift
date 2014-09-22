include "thrift/lib/thrift/dynamic.thrift"

struct Container {
  1: dynamic.Dynamic data
}

service DynamicTestService {
  dynamic.Dynamic echo(1: dynamic.Dynamic input);
}
