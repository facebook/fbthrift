service MyService {
  void ping() (cpp.coroutine)
  string getRandomData()
  bool hasDataById(1: i64 id) (cpp.coroutine)
  string getDataById(1: i64 id) (cpp.coroutine, thread='eb')
  void putDataById(1: i64 id, 2: string data)
}
