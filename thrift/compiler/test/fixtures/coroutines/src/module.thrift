service MyService {
  void ping() (coroutine)
  string getRandomData()
  bool hasDataById(1: i64 id) (coroutine)
  string getDataById(1: i64 id) (coroutine, thread='eb')
  void putDataById(1: i64 id, 2: string data)
}
