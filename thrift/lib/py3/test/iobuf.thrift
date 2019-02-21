cpp_include "folly/io/IOBuf.h"

typedef binary (cpp2.type = "std::unique_ptr<folly::IOBuf>") IOBufPtr
typedef binary (cpp2.type = "folly::IOBuf") IOBuf

struct Moo {
  1: i32 val
  2: IOBufPtr ptr
  3: IOBuf buf
  // this behaves like a mix of a reference and a regular field, so worth
  // testing it specially
  4: optional IOBufPtr opt_ptr
}

service IobufTestService {
    Moo getMoo()
    void sendMoo(1: Moo m)
}
