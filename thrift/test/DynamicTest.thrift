include "thrift/lib/thrift/dynamic.thrift"

struct Container {
  1: dynamic.DynamicType data
}

service DynamicTestService {
  dynamic.DynamicType echo(1: dynamic.DynamicType input);
}
