namespace cpp2 thrift.test.iobufptr

typedef binary (cpp2.type = "std::unique_ptr<folly::IOBuf>") IOBufPtr

struct Request {
  1: IOBufPtr one,
  2: IOBufPtr two = "hello",
}

service IOBufPtrTestService {
  IOBufPtr combine(1: Request req),
}

