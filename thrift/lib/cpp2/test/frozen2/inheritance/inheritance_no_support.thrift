include "thrift/lib/cpp2/test/frozen2/inheritance/inheritance_parent.thrift"

namespace cpp2 test.frozen2

service InheritanceNoSupportService extends inheritance_parent.RootService {
  void stub()
}
