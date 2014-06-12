namespace cpp apache.thrift.test

// get periodic updates service
service DuplexService {
  bool registerForUpdates(1:i32 startIndex, 2:i32 numUpdates, 3:i32 interval)
}

service DuplexClient {
  i32 update(1:i32 currentIndex)
}
