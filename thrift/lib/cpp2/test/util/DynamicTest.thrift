include "thrift/lib/cpp2/util/dynamic.thrift"

namespace cpp2 apache.thrift.util

struct Container {
  1: dynamic.Dynamic data
}

service TestService {
  dynamic.Dynamic echo(1: dynamic.Dynamic input);
}
