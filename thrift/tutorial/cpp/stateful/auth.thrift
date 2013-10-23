exception LoginError {
  1: string message
}

exception PermissionError {
  1: string message
}

exception NoSuchSessionError {
  1: i64 id
}

struct SessionInfo {
  1: i64 id
  2: string username
  3: string clientInfo
  4: i64 openTime
  5: i64 loginTime
}

typedef list<SessionInfo> SessionInfoList

service AuthenticatedService {
  void authenticate(1: string username) throws (1: LoginError loginError)

  SessionInfoList listSessions()
}
