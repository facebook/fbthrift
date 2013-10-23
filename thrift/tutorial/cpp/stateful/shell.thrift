include "thrift/tutorial/cpp/stateful/auth.thrift"

exception OSError {
  1: i32 code
  2: string message
}

struct StatInfo {
  1: string name
  // Other stat info could go here
}

service ShellService extends auth.AuthenticatedService {
  string pwd()
    throws (1: auth.PermissionError permError, 2: OSError osError)
  void chdir(1: string dir)
    throws (1: auth.PermissionError permError, 2: OSError osError)
  list<StatInfo> listDirectory(1: string dir)
    throws (1: auth.PermissionError permError, 2: OSError osError)
  binary cat(1: string file)
    throws (1: auth.PermissionError permError, 2: OSError osError)
}
