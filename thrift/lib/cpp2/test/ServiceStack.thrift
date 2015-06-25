namespace cpp2 apache.thrift.test

service TestServiceStack {
  string sendResponse(1:i64 size)
  oneway void noResponse(1:i64 size)
  string echoRequest(1:string req (cpp.cache))
  string serializationTest(1: bool inEventBase)
  string eventBaseAsync() (thread = 'eb')
  void notCalledBack()
  void voidResponse()
}
