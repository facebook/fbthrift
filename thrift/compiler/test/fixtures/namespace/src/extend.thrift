namespace hs My.Namespacing.Extend.Test

include "hsmodule.thrift"

service ExtendTestService extends hsmodule.HsTestService {
  bool check(
    1: hsmodule.HsFoo struct1,
  )
}
