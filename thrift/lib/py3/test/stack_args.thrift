cpp_include "folly/io/IOBuf.h"

typedef binary (cpp2.type = "std::unique_ptr<folly::IOBuf>") IOBufPtr
typedef binary (cpp2.type = "folly::IOBuf") IOBuf

struct simple {
    1: i32 val,
}

service StackService {
    list<i32> add_to(1: list<i32> lst, 2: i32 value)
    simple get_simple()
    void take_simple(1: simple smpl)
    IOBuf get_iobuf()
    void take_iobuf(1: IOBuf val)
    // currently unsupported by the cpp backend:
    // IOBufPtr get_iobuf_ptr()
    void take_iobuf_ptr(1: IOBufPtr val)
}
