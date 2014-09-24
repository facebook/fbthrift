namespace cpp apache.thrift.test

typedef binary (cpp2.type = "std::unique_ptr<folly::IOBuf>") IOBufPtr
typedef binary (cpp2.type = "folly::IOBuf") IOBuf

exception Xception {
  1: i32 errorCode,
  2: string message
}

service FutureService {
  string sendResponse(1:i64 size)
  oneway void noResponse(1:i64 size)
  string echoRequest(1:string req)
  i32 throwing() throws (1: Xception err1)

  void buftest(1: IOBufPtr data)
}
