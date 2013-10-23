// Copyright 2011 Facebook. All rights reserved.
//
// @author: Wei Chen (weichen@facebook.com)
//
// Thrift template to test TitanThriftServer
//

include "common/fb303/if/fb303.thrift"

namespace java com.facebook.thrift.direct_server.tests

struct SimpleRequest {
  1: bool    true_or_false,
  2: string  value,
}

service JavaSimpleService extends fb303.FacebookService {
  string simple(1: SimpleRequest r)
  string getString(1: i32 size)
}
