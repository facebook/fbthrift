namespace hs My.Namespacing.Extend.Test
namespace py3 my.namespacing.extend.test

include "hsmodule.thrift"

service ExtendTestService extends hsmodule.HsTestService {
  bool check(
    1: hsmodule.HsFoo struct1,
  )
}
