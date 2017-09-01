namespace cpp2 testutil.testservice

exception TestServiceException {
  1: string message
}

service TestService {
  i32 sumTwoNumbers(1: i32 x, 2: i32 y),

  i32 add(1: i32 x),

  /**
   * Throw the expected exception
   */
 void throwExpectedException(1: i32 x)
     throws (1: TestServiceException e),

  /**
   * Throw an unexpected exception
   */
 void throwUnexpectedException(1: i32 x)
   throws (1: TestServiceException e),
}
