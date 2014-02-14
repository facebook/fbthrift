namespace cpp2 thrift.test.iobufptr

typedef binary (cpp2.type = "std::unique_ptr<folly::IOBuf>") IOBufPtr
typedef binary (cpp2.type = "folly::IOBuf") IOBufBinary

struct Request {
  1: IOBufPtr one,
  2: IOBufPtr two = "hello",
  3: IOBufBinary three,
}

service IOBufPtrTestService {
  IOBufPtr combine(1: Request req),
}
