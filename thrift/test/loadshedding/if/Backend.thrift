include "common/fb303/if/fb303.thrift"

namespace cpp2 facebook.thrift.test

struct BackendRequest {
  1: i64 time_per_request
  2: bool consumeCPU = true
}

struct BackendResponse {
  1: string status
}

service BackendService extends fb303.FacebookService {
  BackendResponse doWork(1: BackendRequest request)
}
