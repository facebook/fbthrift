cpp_include "folly/io/IOBuf.h"

typedef binary (cpp2.type = "std::unique_ptr<folly::IOBuf>") IOBufPtr
typedef binary (cpp2.type = "folly::IOBuf") IOBuf

struct Moo {
  1: i32 val
  2: IOBufPtr ptr
  3: IOBuf buf
}

service TestingService {
    Moo getMoo()
    void sendMoo(1: Moo m)
}
