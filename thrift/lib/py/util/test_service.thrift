namespace py thrift.util.test_service

service TestService {
  string getDataById(1: i64 id)
  bool hasDataById(1: i64 id)
  void putDataById(1: i64 id, 2: string data)
  void delDataById(1: i64 id)
}
