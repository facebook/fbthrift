namespace cpp2 testutil.testservice

exception TestServiceException {
  1: string message
}

service TestService {
  i32 sumTwoNumbers(1: i32 x, 2: i32 y),

  i32 add(1: i32 x),

  void throwExpectedException(1: i32 x) throws (1: TestServiceException e),

  void throwUnexpectedException(1: i32 x) throws (1: TestServiceException e),

  void sleep(1: i32 timeMs),

  void headers();
}
