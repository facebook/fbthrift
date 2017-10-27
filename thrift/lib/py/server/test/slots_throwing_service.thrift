namespace py thrift.server.test.slots_throwing_service

exception UserException1 {
  1: string message,
}

exception UserException2 {
  1: string message
}

service SlotsThrowingService {
  void throwUserException()
      throws (1: UserException1 ex1, 2: UserException2 ex2)
  i64 throwUncaughtException(1: string msg)
}
