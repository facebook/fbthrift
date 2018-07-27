typedef binary (cpp2.type = "folly::IOBuf") IOBuf
typedef binary (cpp2.type = "std::unique_ptr<folly::IOBuf>") IOBufPtr
typedef binary (cpp2.type = "folly::fbstring") fbstring_type

struct Binaries {
  1: binary no_special_type
  2: IOBuf iobuf_val
  3: IOBufPtr iobuf_ptr
  4: fbstring_type fbstring
}

service BinaryService {
  Binaries sendRecvBinaries(1: Binaries val)
  binary sendRecvBinary(1: binary val)
  IOBuf sendRecvIOBuf(1: IOBuf val)
  IOBufPtr sendRecvIOBufPtr(1: IOBufPtr val)
  fbstring_type sendRecvFbstring(1: fbstring_type val)
}
