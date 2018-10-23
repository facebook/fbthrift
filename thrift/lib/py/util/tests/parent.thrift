namespace py thrift.test.parent

exception ParentError {
  1: string message
}

service ParentService {
  string getStatus()
}
