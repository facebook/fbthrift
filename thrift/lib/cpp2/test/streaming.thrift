namespace cpp apache.thrift.test

exception StreamingException {
}

service StreamingService {
  stream<i32> streamingMethod(1: i32 count);
  stream<i32> streamingException(1: i32 count) throws (1: StreamingException e);
}
