namespace py thrift.util.test_service

exception UserException1 {
  1: string message,
}

exception UserException2 {
  1: string message
}

service TestService {
  string getDataById(1: i64 id)
  bool hasDataById(1: i64 id)
  void putDataById(1: i64 id, 2: string data)
  void delDataById(1: i64 id)
  void throwUserException()
      throws (1: UserException1 ex1, 2: UserException2 ex2)
  i64 throwUncaughtException(1: string msg)
}

service PriorityService {
  bool bestEffort() (priority='BEST_EFFORT')
  bool normal() (priority='NORMAL')
  bool important() (priority='IMPORTANT')
  bool unspecified()
} (priority='HIGH')

service SubPriorityService extends PriorityService {
  bool child_unspecified()
  bool child_highImportant() (priority='HIGH_IMPORTANT')
}
