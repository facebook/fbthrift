namespace cpp apache.thrift.test

struct TestStruct {
  1: string s,
  2: i32 i,
}

struct TestStructRecursive {
  6: string tag,
  99: optional TestStructRecursive cdr (cpp.ref = 'true'),
}

service TestService {
  string sendResponse(1:i64 size)
  oneway void noResponse(1:i64 size)
  string echoRequest(1:string req (cpp.cache))
  string serializationTest(1: bool inEventBase)
  string eventBaseAsync() (thread = 'eb')
  void notCalledBack()
  void voidResponse()
}
