namespace cpp2 apache.thrift.test

typedef binary (cpp2.type = "std::unique_ptr<folly::IOBuf>") IOBufPtr

struct Binaries {
  1 : binary normal,
  2 : IOBufPtr iobuf,
}
