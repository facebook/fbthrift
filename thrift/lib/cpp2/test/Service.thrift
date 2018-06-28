namespace cpp2 apache.thrift.test

struct TestStruct {
  1: string s,
  2: i32 i,
}

typedef binary (cpp2.type = "folly::IOBuf") IOBuf
typedef binary (cpp2.type = "std::unique_ptr<folly::IOBuf>") IOBufPtr

struct TestStructIOBuf {
  1: IOBuf buf,
  2: i32 i,
}

struct TestStructRecursive {
  6: string tag,
  99: optional TestStructRecursive cdr (cpp.ref = 'true'),
}

typedef byte (cpp2.type = "uint8_t") UInt8
typedef i16 (cpp2.type = "uint16_t") UInt16
typedef i32 (cpp2.type = "uint32_t") UInt32
typedef i64 (cpp2.type = "uint64_t") UInt64

struct TestUnsignedIntStruct {
  1: UInt8 u8,
  2: UInt16 u16,
  3: UInt32 u32,
  4: UInt64 u64
}

union TestUnsignedIntUnion {
  1: UInt8 u8,
  2: UInt16 u16,
  3: UInt32 u32,
  4: UInt64 u64
}

struct TestUnsignedInt32ListStruct {
  1: list<UInt32> l
}

struct TestUnsignedIntMapStruct {
  1: map<UInt32, UInt64> m
}

service TestService {
  string sendResponse(1:i64 size)
  oneway void noResponse(1:i64 size)
  string echoRequest(1:string req (cpp.cache))
  i32 echoInt(1:i32 req)
  string serializationTest(1: bool inEventBase)
  string eventBaseAsync() (thread = 'eb')
  void notCalledBack()
  void voidResponse()
  i32 processHeader()
  IOBufPtr echoIOBuf(1: IOBuf buf)
}
