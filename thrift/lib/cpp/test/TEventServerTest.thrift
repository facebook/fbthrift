namespace cpp apache.thrift.test

service TEventServerTestService {
  string sendResponse(1:i64 size)

  void noop()
}
